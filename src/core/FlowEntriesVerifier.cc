/*
 * Copyright 2019 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FlowEntriesVerifier.hpp"

#include "SwitchManager.hpp"
#include "Recovery.hpp"
#include "DatabaseConnector.hpp"
#include "Controller.hpp"
#include "Logger.hpp"
#include "api/Switch.hpp"
#include "api/OFAgent.hpp"

#include <of13/of13match.hh>

#include <boost/functional/hash.hpp>

#include <unordered_set>
#include <functional>
#include <vector>
#include <mutex>

namespace runos {

REGISTER_APPLICATION(FlowEntriesVerifier, {"controller", "switch-ordering",
                                           "switch-manager", "recovery-manager",
                                           "database-connector", ""})

using lock_t = std::lock_guard<std::mutex>;
using shared_lock_t = boost::shared_lock<boost::shared_mutex>;
using upgrade_lock_t = boost::upgrade_lock<boost::shared_mutex>;
using unique_lock_t = boost::upgrade_to_unique_lock<boost::shared_mutex>;

using FlowModPtrSequence = std::vector<FlowModPtr>;
using FlowStatsSequence = std::vector<of13::FlowStats>;

static constexpr size_t OXM_FIELD_VALUE_SIZE = 2 * sizeof(uint32_t);
static constexpr size_t OXM_FIELD_SIZE =
    of13::OFP_OXM_HEADER_LEN + OXM_FIELD_VALUE_SIZE;

namespace hash {
    static size_t hashValue(of13::Match&& m)
    {
        size_t hash = 0;
        size_t match_size = m.length() + m.oxm_fields_len() * OXM_FIELD_SIZE;

        std::vector<uint8_t> buf(match_size);
        m.pack(buf.data());
        for (size_t i = 0; i < match_size; ++i) {
            boost::hash_combine(hash, buf[i]);
        }
        return hash;
    }

    template<class T>
    static size_t flowHashValue(T& msg)
    {
        size_t hash = hashValue(msg.match());

        boost::hash_combine(hash, msg.table_id());
        boost::hash_combine(hash, msg.priority());

        return hash;
    }
}

class PatternData {
public:
    explicit PatternData(of13::FlowMod& fm)
        : table_id_(fm.table_id())
        , priority_(fm.priority())
        , out_group_(fm.out_group())
        , out_port_(fm.out_port())
        , cookie_(fm.cookie())
        , cookie_mask_(fm.cookie_mask())
        , match_(fm.match())
    { }

    bool toBeModifiedBase(FlowModPtr &fmp) const
    {
        return fmp->table_id() == table_id_ && cookie_filter(fmp);
    }

    bool toBeDeletedBase(FlowModPtr &fmp) const
    {
        bool table_id = is_table_id_all() ||
                        (!is_table_id_all() || fmp->table_id() == table_id_);
        bool out_port = !is_out_port_any() || fmp->out_port() == out_port_;
        bool out_group = !is_out_group_any() || fmp->out_group() == out_group_;

        return toBeModifiedBase(fmp) && table_id && out_port && out_group;
    }

    bool nonStrictMatch(FlowModPtr &fmp) const
    {
        auto&& fmp_match = fmp->match();

        for (uint8_t field = 0; field < OXM_NUM; ++field) {
            of13::OXMTLV* field_ptr = fmp_match.oxm_field(field);
            of13::OXMTLV* match_ptr = match_.oxm_field(field);

            if (match_ptr != nullptr &&
                (field_ptr == nullptr || !field_ptr->equals(*match_ptr))) {
                return false;
            }
        }
        return true;
    }

    bool strictMatch(FlowModPtr &fmp) const
    {
        return fmp->priority() == priority_ &&
               fmp->match() == match_;
    }

private:
    uint8_t table_id_;
    uint16_t priority_;
    uint32_t out_group_;
    uint32_t out_port_;
    uint64_t cookie_;
    uint64_t cookie_mask_;
    mutable of13::Match match_;

    bool is_table_id_all() const { return table_id_ == of13::OFPTT_ALL; }
    bool is_out_port_any() const { return out_port_ == of13::OFPP_ANY; }
    bool is_out_group_any() const { return out_group_ == of13::OFPG_ANY; }

    bool cookie_filter(FlowModPtr& fmp) const
    {
        return (fmp->cookie() & cookie_mask_) == (cookie_ & cookie_mask_);
    }
};

class Pattern {
public:
    explicit Pattern(of13::FlowMod& fm)
        : pattern_data_(fm)
        , command_(fm.command())
    { }

    bool matches(FlowModPtr& fmp) const
    {
        switch (command_) {
            case of13::OFPFC_MODIFY:
                return to_be_modified(fmp);
            case of13::OFPFC_DELETE:
                return to_be_deleted(fmp);
            case of13::OFPFC_MODIFY_STRICT:
                return to_be_modified_strictly(fmp);
            case of13::OFPFC_DELETE_STRICT:
                return to_be_deleted_strictly(fmp);
            default:
                return false;
        }
    }

private:
    PatternData pattern_data_;
    uint8_t command_;

    bool to_be_modified(FlowModPtr& fmp) const
    {
        return pattern_data_.toBeModifiedBase(fmp) &&
                pattern_data_.nonStrictMatch(fmp);
    }

    bool to_be_deleted(FlowModPtr& fmp) const
    {
        return pattern_data_.toBeDeletedBase(fmp) &&
                pattern_data_.nonStrictMatch(fmp);
    }

    bool to_be_modified_strictly(FlowModPtr& fmp) const
    {
        return pattern_data_.toBeModifiedBase(fmp) &&
                pattern_data_.strictMatch(fmp);
    }

    bool to_be_deleted_strictly(FlowModPtr& fmp) const
    {
        return pattern_data_.toBeDeletedBase(fmp) &&
                pattern_data_.strictMatch(fmp);
    }
};

class FlowMessage {
public:
    template<class T>
    explicit FlowMessage(T& msg)
        : fmp_(std::make_unique<of13::FlowMod>())
    { }

    explicit FlowMessage(of13::FlowMod& fm)
        : fmp_(copy(fm))
    { }

    explicit FlowMessage(const json& flow_json)
        : fmp_(parse(flow_json))
    { }

    FlowMessage(const FlowMessage& rhs)
        : fmp_(copy(*rhs.fmp_))
    { }

    ~FlowMessage() noexcept = default;

    FlowModPtr ptr() const { return copy(*fmp_); }
    of13::InstructionSet instructions() const { return fmp_->instructions(); }
    json toJson() const { return dump(); }

    void changeInstructions(const of13::InstructionSet& is)
    {
        fmp_->instructions(is);
    }

    bool matches(const Pattern& pv) const { return pv.matches(fmp_); }

private:
    mutable FlowModPtr fmp_;

    using raw_t = std::unique_ptr<uint8_t>;
    using raw_vector_t = std::vector<uint8_t>;

    json dump() const
    {
        auto dump = json::array();
        raw_t packed{ fmp_->pack() };
        auto dump_size = fmp_->length();

        for (auto i = 0; i < dump_size; ++i) {
            uchar value = *(packed.get() + i);
            dump.push_back(value);
        }

        VLOG(17) << "[FlowEntriesVerifier] Dumped Flow-Mod message is of type="
                    << static_cast<unsigned>(fmp_->type())
                 << ", of size=" << fmp_->length()
                 << " and refers to the table="
                    << static_cast<unsigned>(fmp_->table_id())
                 << " with priority=" << fmp_->priority();

        return dump;
    }

    static FlowModPtr parse(const json& fm_dump)
    {
        FlowModPtr fmp = std::make_unique<of13::FlowMod>();
        raw_vector_t parsed;

        for (auto value: fm_dump) {
            parsed.push_back(std::move(value));
        }
        fmp->unpack(parsed.data());

        VLOG(17) << "[FlowEntriesVerifier] Parsed Flow-Mod message is of type="
                    << static_cast<unsigned>(fmp->type())
                 << ", of size=" << fmp->length()
                 << " and refers to the table="
                     << static_cast<unsigned>(fmp->table_id())
                 << " with priority=" << fmp->priority();

        return fmp;
    }

    static FlowModPtr copy(of13::FlowMod& fm)
    {
        auto copy = std::make_unique<of13::FlowMod>();
        raw_t packed{ fm.pack() };
        copy->unpack(packed.get());
        return copy;
    }
};

class Flow {
public:
    template<class T>
    explicit Flow(T& msg)
        : msg_(msg)
        , table_id_(msg.table_id())
        , priority_(msg.priority())
        , match_(msg.match())
        , hash_(hash::flowHashValue(msg))
    { }

    explicit Flow(const json& flow_message_json)
        : msg_(flow_message_json)
    {
        auto fmp = msg_.ptr();
        table_id_ = fmp->table_id();
        priority_ = fmp->priority();
        match_ = fmp->match();
        hash_ = hash::flowHashValue(*fmp);
    }

    explicit Flow(const Flow& flow, const of13::InstructionSet& is)
        : Flow(flow)
    {
        msg_.changeInstructions(is);
    }

    Flow(const Flow& rhs) = default;
    ~Flow() noexcept = default;

    bool operator==(const Flow& rhs) const
    {
        return table_id_ == rhs.table_id_ &&
               priority_ == rhs.priority_ &&
               match_ == rhs.match_;
    }

    FlowMessage message() const { return msg_; }
    of13::InstructionSet instructions() const { return msg_.instructions(); }
    json toJson() const { return msg_.toJson(); }

    bool matches(const Pattern& pv) const { return msg_.matches(pv); }

    struct Hasher {
        size_t operator()(const Flow& flow) const { return flow.hash_; }
    };

private:
    FlowMessage msg_;
    uint8_t table_id_;
    uint16_t priority_;
    of13::Match match_;
    size_t hash_;
};

using FlowSet = std::unordered_set<Flow, Flow::Hasher>;

class FlowModHandler {
public:
    explicit FlowModHandler(of13::FlowMod& fm)
        : flow_(fm)
        , pattern_(fm)
        , command_(fm.command())
    { }

    void applyToFlowSet(FlowSet& flow_entries) const
    {
        switch (command_) {
            case of13::OFPFC_ADD:
                add_to_flow_set(flow_entries);
                break;
            case of13::OFPFC_MODIFY:
            case of13::OFPFC_MODIFY_STRICT:
                modify_flow_set(flow_entries);
                break;
            case of13::OFPFC_DELETE:
            case of13::OFPFC_DELETE_STRICT:
                delete_from_flow_set(flow_entries);
                break;
            default:
                break;
        }
    }

private:
    Flow flow_;
    Pattern pattern_;
    uint8_t command_;

    void add_to_flow_set(FlowSet& flows) const { flows.insert(flow_); }

    void modify_flow_set(FlowSet& flows) const
    {
        FlowSet modified_entries;

        for (auto it = flows.begin(); it != flows.end(); ++it) {
            if (!it->matches(pattern_)) {
                continue;
            }

            auto instr = flow_.instructions();
            modified_entries.emplace(*it, instr);
            it = flows.erase(it);
        }

        flows.insert(modified_entries.begin(), modified_entries.end());
    }

    void delete_from_flow_set(FlowSet& flows) const
    {
        for (auto it = flows.begin(); it != flows.end(); ) {
            if (it->matches(pattern_)) {
                it = flows.erase(it);
            } else {
                ++it;
            }
        }
    }
};

class SwitchState {
public:
    SwitchState() noexcept = default;
    explicit SwitchState(const json& state_json) { fromJson(state_json); }

    void process(of13::FlowMod& fm)
    {
        FlowModHandler handler(fm);

        lock_t lock(entries_mut_);
        handler.applyToFlowSet(flow_entries_);
    }

    FlowModPtr process(of13::FlowRemoved& fr)
    {
        Flow removed_flow(fr);

        lock_t lock(entries_mut_);
        auto it = flow_entries_.find(removed_flow);
        if (it != flow_entries_.end()) {
            if (is_expected(fr)) {
                flow_entries_.erase(it);
            } else {
                auto&& msg = it->message();
                return msg.ptr();
            }
        }
        return nullptr;
    }

    FlowModPtrSequence process(FlowStatsSequence& fs_seq) const
    {
        auto&& flow_set = to_flow_set(fs_seq);
        FlowModPtrSequence fmp_sequence;

        lock_t lock(entries_mut_);
        for (auto& flow: flow_entries_) {
            auto it = flow_set.find(flow);
            if (it == flow_set.end()) {
                auto&& msg = flow.message();
                fmp_sequence.push_back(msg.ptr());
            }
        }

        return fmp_sequence;
    }

    json toJson() const
    {
        lock_t lock(entries_mut_);

        auto&& state_json = json::array();
        for (const auto& flow: flow_entries_) {
            state_json.push_back(flow.toJson());
        }
        return state_json;
    }

    void fromJson(const json& state_json)
    {
        lock_t lock(entries_mut_);

        flow_entries_.clear();
        for (const auto& flow_json: state_json) {
            flow_entries_.emplace(flow_json);
        }
    }

private:
    FlowSet flow_entries_;
    mutable std::mutex entries_mut_;

    static FlowSet to_flow_set(FlowStatsSequence& fs_sequence)
    {
        FlowSet flow_set;
        for (auto& fs: fs_sequence) {
            flow_set.emplace(fs);
        }
        return flow_set;
    }

    static bool is_expected(of13::FlowRemoved& fr)
    {
        auto reason = fr.reason();
        return reason == of13::OFPRR_IDLE_TIMEOUT ||
               reason == of13::OFPRR_HARD_TIMEOUT ||
               reason == of13::OFPRR_GROUP_DELETE ||
               reason == of13::OFPRR_METER_DELETE;
    }
};

class MessageSender {
public:
    explicit MessageSender(SwitchManager* sw_mgr) noexcept
        : sw_mgr_(sw_mgr)
    { }

    void send(uint64_t dpid, fluid_msg::OFMsg& msg) const
    {
        auto conn = sw_mgr_->switch_(dpid)->connection();
        conn->send(msg);
    }

    void send(uint64_t dpid, FlowModPtr& fmp) const { send(dpid, *fmp); }

    void send(uint64_t dpid, FlowModPtrSequence& fmp_sequence) const
    {
        auto conn = sw_mgr_->switch_(dpid)->connection();
        for (auto& fmp: fmp_sequence) {
            conn->send(*fmp);
        }
    }

    FlowStatsSequence flowStatsRequest(uint64_t dpid) const
    {
        UnsafeSwitchPtr sw = sw_mgr_->switch_(dpid);
        auto agent = sw->connection()->agent();
        ofp::flow_stats_request req;

        try {
            auto f = agent->request_flow_stats(req);
            auto status = f.wait_for(boost::chrono::seconds(5));
            if (status == boost::future_status::timeout) {
                return FlowStatsSequence();
            }
            return f.get();
        }
        catch (OFAgent::request_error const& e) {
            return FlowStatsSequence();
        }
    }

private:
    SwitchManager* sw_mgr_;
};

class Recovery {
public:
    using dump_json = VerifierDatabase::dump_json;

    explicit Recovery(DatabaseConnector* db_mgr, RecoveryManager* rc_mgr)
        : db_mgr_(db_mgr)
        , rc_mgr_(rc_mgr)
    { }

    void save(const dump_json& db_dump) const
    {
        json states_list;
        for (const auto& state_pair: db_dump) {
            states_list.push_back(state_pair.first);
        }
        save_states_list(states_list);

        for (const auto& state_pair: db_dump) {
            save_state(state_pair.first, state_pair.second);
        }
    }

    dump_json load() const
    {
        dump_json db_dump;

        auto&& states_list = get_states_list();
        for (const std::string& dpid_str: states_list) {
            db_dump[dpid_str] = get_state(dpid_str);
        }

        return db_dump;
    }

    bool isPrimary() const { return rc_mgr_->isPrimary(); }

private:
    DatabaseConnector* db_mgr_;
    RecoveryManager* rc_mgr_;

    static constexpr auto states_prefix = "flow-entries-verifier:state";
    static constexpr auto settings_prefix = "flow-entries-verifier";
    static constexpr auto states_list_key = "states_list";

    static std::string state_prefix(const std::string& dpid_str)
    {
        return std::string(states_prefix) + ":" + dpid_str;
    }

    void save_states_list(const json& list) const
    {
        db_mgr_->putSValue(settings_prefix, states_list_key, list.dump());
        VLOG(30) << "[FlowEntriesVerifier] Saved states list to db: "
                 << list.dump();
    }

    void save_state(const std::string& dpid_str, const json& state) const
    {
        VLOG(17) << "[FlowEntriesVerifier] Saving state #"
                 << dpid_str << ": " << state;

        for (size_t i = 1; i < state.size() + 1; ++i) {
            const auto& flow = state[i - 1];
            db_mgr_->putSValue(state_prefix(dpid_str),
                               std::to_string(i),
                               flow.dump());
        }
    }

    json get_states_list() const
    {
        auto&& list_dump = db_mgr_->getSValue(settings_prefix, states_list_key);

        VLOG(30) << "[FlowEntriesVerifier] Got states list from db: "
                 << list_dump;

        if (list_dump.empty()) {
            return json::array();
        }

        return json::parse(list_dump);
    }

    json get_state(const std::string dpid_str) const
    {
        auto&& state_keys = db_mgr_->getKeys(state_prefix(dpid_str));

        if (state_keys.empty()) {
            return json::array();
        }

        json state;
        for (const auto& key: state_keys) {
            auto&& flow = db_mgr_->getSValue(state_prefix(dpid_str), key);
            if (flow.empty()) {
                continue;
            }

            state.push_back(json::parse(flow));
        }
        VLOG(17) << "[FlowEntriesVerifier] Got state #"
                 << dpid_str << " from db: " << state;

        return state;
    }
};

/*  VERIFIER DATABASE  */

void VerifierDatabase::removeState(uint64_t dpid)
{
    upgrade_lock_t lock(states_mut_);
    unique_lock_t unique_lock(lock);

    auto it = states_.find(dpid);
    CHECK(it != states_.end());
    states_.erase(it);
}

void VerifierDatabase::process(uint64_t dpid, of13::FlowMod& fm)
{
    SwitchStatePtr state_ptr;

    { //lock
        shared_lock_t lock(states_mut_);
        auto it = states_.find(dpid);
        if (it == states_.end()) {
            VLOG(5) << "[FlowEntriesVerifier] Processing Flow-Mod "
                    << "to unknown switch dpid=" << dpid;
            return;
        }
        state_ptr = it->second;
    } //unlock

    state_ptr->process(fm);
}

FlowModPtr VerifierDatabase::process(uint64_t dpid, of13::FlowRemoved& fr)
{
    VLOG(8) << "[FlowEntriesVerifier] Flow-Removed arrived from switch "
            << "dpid=" << dpid;

    auto state_ptr = find_state(dpid);
    if (!state_ptr) {
        VLOG(6) << "[FlowEntriesVerifier] Flow-Removed from unknown "
               << "switch dpid=" << dpid << " arrived";
        return nullptr;
    }

    return state_ptr->process(fr);
}

void VerifierDatabase::fromJson(VerifierDatabase::dump_json& db_dump)
{
    clear();
    for (const auto& state_pair: db_dump) {
        auto dpid = std::stoull(state_pair.first);
        auto& state = state_pair.second;

        addState(dpid, state);
    }
}

VerifierDatabase::dump_json VerifierDatabase::toJson() const
{
    dump_json db_dump;

    shared_lock_t lock(states_mut_);
    for (const auto& state: states_) {
        std::string dpid_str = std::to_string(state.first);
        db_dump[dpid_str] = state.second->toJson();
    }
    return db_dump;
}

void VerifierDatabase::restoreStates(const MessageSender* sender) const
{
    shared_lock_t lock(states_mut_);

    for (const auto& pair: states_) {
        auto dpid = pair.first;
        auto& state_ptr = pair.second;

        auto&& flow_stats = sender->flowStatsRequest(dpid);
        if (!flow_stats.empty()) {
            auto&& fmp_sequence = state_ptr->process(flow_stats);
            if (!fmp_sequence.empty()) {
                LOG(WARNING) << "[FlowEntriesVerifier] No "
                             << fmp_sequence.size() << "required flow entries "
                             << "on switch dpid=" << dpid;
            }

            sender->send(dpid, fmp_sequence);

            if (!fmp_sequence.empty()) {
                VLOG(7) << "[FlowEntriesVerifier] " << fmp_sequence.size()
                        << "Flow-Mod re-sent to switch dpid=" << dpid;
            }
        }
    }
}

void VerifierDatabase::add_state_impl(uint64_t dpid, SwitchStatePtr& state_ptr)
{
    upgrade_lock_t lock(states_mut_);
    unique_lock_t unique_lock(lock);

    auto&& pair = states_.emplace(dpid, state_ptr);
    auto& state_it = pair.first;
    if (!pair.second) {
        state_it->second = state_ptr;
    }
}

void VerifierDatabase::clear()
{
    upgrade_lock_t lock(states_mut_);
    unique_lock_t unique_lock(lock);
    states_.clear();

    VLOG(8) << "[FlowEntriesVerifier] VerifierDatabase cleared";
}

VerifierDatabase::SwitchStatePtr VerifierDatabase::find_state(uint64_t dpid) const
{
    shared_lock_t lock(states_mut_);

    auto it = states_.find(dpid);
    if (it == states_.end()) {
        return nullptr;
    }
    return it->second;
}

/*   IMPLEMENTATION   */

struct FlowEntriesVerifier::implementation final
    : OFMessageHandler<of13::FlowRemoved>
{
    VerifierDatabase* data_ptr;
    MessageSender sender;
    Recovery recovery;

    explicit implementation(VerifierDatabase* data, SwitchManager* sw_mgr,
                            DatabaseConnector* db_mgr, RecoveryManager* rc_mgr)
        : data_ptr(data)
        , sender(sw_mgr)
        , recovery(db_mgr, rc_mgr)
    { }

    void send(uint64_t dpid, fluid_msg::OFMsg& msg) { sender.send(dpid, msg); }

    void send(uint64_t dpid, of13::FlowMod* fmp)
    {
        fmp->flags(fmp->flags() | of13::OFPFF_SEND_FLOW_REM);
        send(dpid, *fmp);
        process(*fmp, dpid);
    }

    void polling() const
    {
        if (!isPrimary()) {
            return;
        }

        saveToDatabase();
        restoreStates();
    }

    void switchDown(uint64_t dpid)
    {
        if (!isPrimary()) {
            return;
        }

        data_ptr->removeState(dpid);
    }

    void switchUp(uint64_t dpid)
    {
        if (!isPrimary()) {
            return;
        }

        data_ptr->addState(dpid);
    }

    bool process(of13::FlowRemoved& fr, OFConnectionPtr conn) override
    {
        if (!isPrimary()) {
            return false;
        }

        auto dpid = conn->dpid();
        auto&& fmp = data_ptr->process(dpid, fr);
        if (fmp) {
            VLOG(7) << "[FlowEntriesVerifier] Unexpected flow removal "
                    << "on switch dpid=" << dpid << ", table id="
                    << static_cast<int>(fr.table_id());

            sender.send(dpid, fmp);

            VLOG(7) << "[FlowEntriesVerifier] Flow-Mod re-sent "
                    << "to switch dpid=" << dpid;
        }
        return false;
    }

    void process(of13::FlowMod& fm, uint64_t dpid)
    {
        CHECK(isPrimary());
        data_ptr->process(dpid, fm);
    }

    void loadFromDatabase()
    {
        auto&& db_dump = recovery.load();
        data_ptr->fromJson(db_dump);
    }

    void saveToDatabase() const
    {
        auto&& db_dump = data_ptr->toJson();
        recovery.save(db_dump);

        VLOG(6) << "[FlowEntriesVerifier] States were saved to database";
    }

    void restoreStates() const
    {
        data_ptr->restoreStates(&sender);

        VLOG(6) << "[FlowEntriesVerifier] States were verified";
    }

    bool isPrimary() const { return recovery.isPrimary(); }
};

/*     APPLICATION    */

void FlowEntriesVerifier::init(Loader* loader, const Config& rootConfig)
{
    auto config = config_cd(rootConfig, "flow-entries-verifier");
    is_active_ = config_get(config, "active", false);

    SwitchManager* sw_mgr = SwitchManager::get(loader);
    RecoveryManager* rc_mgr = RecoveryManager::get(loader);
    DatabaseConnector* db_mgr = DatabaseConnector::get(loader);
    impl_.reset(new implementation(&data_, sw_mgr, db_mgr, rc_mgr));

    if (is_active_) {
        uint16_t poll_interval = config_get(config, "poll-interval", 30000);
        poller_.reset(new Poller(this, poll_interval));

        Controller::get(loader)->register_handler(impl_, -92);
        SwitchOrderingManager::get(loader)->registerHandler(this, 17);

        connect(rc_mgr, &RecoveryManager::signalRecovery, this, [this]() {
            impl_->loadFromDatabase();
            LOG(INFO) << "[FlowEntriesVerifier] FlowEntriesVerifier recovered";
        });

        connect(rc_mgr, &RecoveryManager::signalSetupBackupMode,
                this, [this]() { data_.clear(); });
        connect(rc_mgr, &RecoveryManager::signalSetupPrimaryMode,
                this, [this]() { impl_->loadFromDatabase(); });
    }
}

void FlowEntriesVerifier::startUp(Loader *loader)
{
    if (is_active_) {
        poller_->run();
    }
}

void FlowEntriesVerifier::send(uint64_t dpid, fluid_msg::OFMsg& msg)
{
    if (is_active_ && msg.type() == of13::OFPT_FLOW_MOD) {
        auto fmp = dynamic_cast<of13::FlowMod*>(&msg);
        impl_->send(dpid, fmp);
    } else {
        impl_->send(dpid, msg);
    }
}

void FlowEntriesVerifier::polling()
{
    if (is_active_) {
        impl_->polling();
    }
}

void FlowEntriesVerifier::switchUp(SwitchPtr sw)
{
    impl_->switchUp(sw->dpid());
}

void FlowEntriesVerifier::switchDown(SwitchPtr sw)
{
    impl_->switchDown(sw->dpid());
}

} // namespace runos
