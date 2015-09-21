/*
 * Copyright 2015 Applied Research Center for Computer Networks
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

#include "StaticFlowPusher.hh"

#include "Controller.hh"
#include "FlowManager.hh"
#include "RestListener.hh"

REGISTER_APPLICATION(StaticFlowPusher, {"controller", "switch-manager", "rest-listener", "flow-manager", ""})

constexpr auto TO_CONTROLLER = "to-controller";

class StaticFlow : public Flow {
public:
    StaticFlow() : Flow(nullptr) {}
    void setLive() {
        Flow::setLive();
    }
};

struct FlowDescImpl {
    uint32_t in_port{0};
    uint32_t out_port{0};
    uint16_t eth_type{0};
    EthAddress eth_src{EthAddress()};
    EthAddress eth_dst{EthAddress()};
    IPAddress ip_src{IPAddress()};
    IPAddress ip_dst{IPAddress()};

    int idle{0};
    int hard{0};
    uint16_t priority{0};

    ModifyList modify;
};
 
FlowDesc::FlowDesc()
{ m = new FlowDescImpl(); }

uint32_t FlowDesc::in_port()
{ return m->in_port; }

uint32_t FlowDesc::out_port()
{ return m->out_port; }

uint16_t FlowDesc::eth_type()
{ return m->eth_type; }

EthAddress FlowDesc::eth_src()
{ return m->eth_src; }

EthAddress FlowDesc::eth_dst()
{ return m->eth_dst; }

IPAddress FlowDesc::ip_src()
{ return m->ip_src; }

IPAddress FlowDesc::ip_dst()
{ return m->ip_dst; }

int FlowDesc::idle()
{ return m->idle; }

int FlowDesc::hard()
{ return m->hard; }

uint16_t FlowDesc::priority()
{ return m->priority; }

void FlowDesc::in_port(uint32_t port)
{ m->in_port = port; }

void FlowDesc::out_port(uint32_t port)
{ m->out_port = port; }

void FlowDesc::eth_type(uint16_t type)
{ m->eth_type = type; }

void FlowDesc::eth_src(std::string mac)
{ m->eth_src = EthAddress(mac); }

void FlowDesc::eth_dst(std::string mac)
{ m->eth_dst = EthAddress(mac); }

void FlowDesc::ip_src(std::string ip)
{ m->ip_src = IPAddress(ip); }

void FlowDesc::ip_dst(std::string ip)
{ m->ip_dst = IPAddress(ip); }

void FlowDesc::idle(int time)
{ m->idle = time; }

void FlowDesc::hard(int time)
{ m->hard = time; }

void FlowDesc::priority(int prio)
{ m->priority = prio; }

void FlowDesc::modifyField(ModifyElem elem)
{ m->modify.push_back(elem); }

ModifyList FlowDesc::modify()
{ return m->modify; }

void StaticFlowPusher::init(Loader* loader, const Config& rootConfig)
{
    Controller* ctrl = Controller::get(loader);
    new_flow = ctrl->registerStaticTransaction(this);

    start_prio = 1;
    flow_m = FlowManager::get(loader);
    sw_m = SwitchManager::get(loader);
    connect(sw_m, &SwitchManager::switchDiscovered, this, &StaticFlowPusher::onSwitchDiscovered);
    connect(sw_m, &SwitchManager::switchDown, this, &StaticFlowPusher::onSwitchDown);

    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::POST, "newflow/[0-9]+");

    auto config = config_cd(rootConfig, "static-flow-pusher");
    def_act = config_get(config, "default", "nope");
    if (config.find("flows") != config.end()) {
        auto static_flows = config.at("flows");
        for (auto& flow_for_switch : static_flows.array_items()) {
            Config cc = flow_for_switch.object_items();
            std::string dpid = config_get(cc, "dpid", "none");
            if (dpid != "none")
                flows_map[dpid] = cc.at("flows");
        }
    }
}

of13::FlowMod StaticFlowPusher::formFlowMod(FlowDesc* fd, Switch* sw)
{
    StaticFlow* sf = new StaticFlow;

    of13::FlowMod fm;
    fm.command(of13::OFPFC_ADD);
    fm.buffer_id(OFP_NO_BUFFER);
    //fm.flags(of13::OFPFF_CHECK_OVERLAP);

    if (fd->in_port() > 0) {
        of13::InPort* oxm = new of13::InPort(fd->in_port());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
    }
    if (fd->eth_src().to_string() != "00:00:00:00:00:00") {
        of13::EthSrc* oxm = new of13::EthSrc(fd->eth_src());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
    }
    if (fd->eth_dst().to_string() != "00:00:00:00:00:00") {
        of13::EthDst* oxm = new of13::EthDst(fd->eth_dst());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
    }

    if (fd->ip_src().getIPv4() != IPAddress::IPv4from_string("0.0.0.0")) {
        of13::IPv4Src* oxm = new of13::IPv4Src(fd->ip_src());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
        fd->eth_type(0x0800);
    }
    if (fd->ip_dst().getIPv4() != IPAddress::IPv4from_string("0.0.0.0")) {
        of13::IPv4Dst* oxm = new of13::IPv4Dst(fd->ip_dst());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
        fd->eth_type(0x0800);
    }

    of13::ApplyActions act;
    if (not fd->modify().empty()) {
        for (auto it : fd->modify()) {
            of13::SetFieldAction* set = new of13::SetFieldAction(it);
            if (it->field() == of13::OFPXMT_OFB_IPV4_SRC || it->field() == of13::OFPXMT_OFB_IPV4_DST) {
                    fd->eth_type(0x0800);
            }
            act.add_action(set);
            sf->add_action(set);
        }
    }

    if (fd->eth_type() > 0) {
        of13::EthType* oxm = new of13::EthType(fd->eth_type());
        fm.add_oxm_field(oxm);
        sf->toTrace(TraceEntry::Load, oxm);
    }

    fm.idle_timeout(fd->idle());
    fm.hard_timeout(fd->hard());
    if (fd->priority()) {
        fm.priority(fd->priority());
    }
    else {
        fm.priority(start_prio++);
    }

    if (fd->out_port() > 0) {
        of13::OutputAction* out = new of13::OutputAction(fd->out_port(), 128);
        act.add_action(out);
        sf->add_action(out);
    }


    fm.add_instruction(act);

    flow_m->addToFlowManager(sf, sw->id());

    return fm;
}

FlowDesc StaticFlowPusher::readFlowFromConfig(Config config)
{
    FlowDesc fd;
    fd.in_port(config_get(config, "in_port", 0));

    uint32_t out_port = config_get(config, "out_port", 0);
    if (out_port == 0) {
        std::string to_ctrl = config_get(config, "out_port", "null");
        if (to_ctrl == TO_CONTROLLER)
            out_port = of13::OFPP_CONTROLLER;
    }
    fd.out_port(out_port);

    fd.eth_src(config_get(config, "eth_src", "00:00:00:00:00:00"));
    fd.eth_dst(config_get(config, "eth_dst", "00:00:00:00:00:00"));
    fd.ip_src(config_get(config, "ip_src", "0.0.0.0"));
    fd.ip_dst(config_get(config, "ip_dst", "0.0.0.0"));
    fd.idle(config_get(config, "idle", 0));
    fd.hard(config_get(config, "hard", 0));
    fd.priority(config_get(config, "priority", 0));

    return fd;
}

void StaticFlowPusher::cleanFlowTable(OFConnection* ofconn)
{
    of13::FlowMod fm;
    fm.table_id(0);
    fm.command(of13::OFPFC_DELETE);
    fm.out_port(of13::OFPP_ANY);
    fm.out_group(of13::OFPG_ANY);

    uint8_t* buf = fm.pack();
    ofconn->send(buf, fm.length());
    OFMsg::free_buffer(buf);
}

void StaticFlowPusher::sendDefault(Switch *sw)
{
    of13::FlowMod fm;
    of13::GoToTable go_to_trace(1);
    fm.add_instruction(go_to_trace);
    sw->send(&fm);

    if (def_act == "to-controller") {
        of13::FlowMod def;
        def.table_id(1);
        of13::ApplyActions act;
        of13::OutputAction* out = new of13::OutputAction(of13::OFPP_CONTROLLER, 128);
        act.add_action(out);
        def.add_instruction(act);
        sw->send(&def);
    }
}

void StaticFlowPusher::sendToSwitch(Switch* dp, FlowDesc* fd)
{
    of13::FlowMod fm = formFlowMod(fd, dp);
    dp->send(&fm);
}

void StaticFlowPusher::onSwitchDiscovered(Switch *dp)
{
    cleanFlowTable(dp->ofconn());
    sendDefault(dp);
    if (flows_map.count("all") > 0) {
        json11::Json flows = flows_map["all"];
        for (auto& flow : flows.array_items()) {
            Config flow_desc = flow.object_items();
            FlowDesc fd = readFlowFromConfig(flow_desc);
            sendToSwitch(dp, &fd);
        }
    }

    if (flows_map.count(dp->idstr()) > 0) {
        json11::Json flows = flows_map[dp->idstr()];
        for (auto& flow : flows.array_items()) {
            Config flow_desc = flow.object_items();
            FlowDesc fd = readFlowFromConfig(flow_desc);
            sendToSwitch(dp, &fd);
        }
    }
}

void StaticFlowPusher::onSwitchDown(Switch *dp)
{
    //TODO: might be useful for further releases
}

#define handle_string(field) \
    if (flow_object.find(#field) != flow_object.end()) { \
        std::string oxm = flow_object[#field].string_value(); \
        if (oxm != "") { \
            fd.field(oxm); \
        } \
    }

#define handle_int(field) \
    if (flow_object.find(#field) != flow_object.end()) { \
        int oxm = flow_object[#field].number_value(); \
        if (oxm > 0) { \
            fd.field(oxm); \
        } \
    }

#define handle_modify_eth(mod_field, oxm_field) \
    if (flow_object.find(#mod_field) != flow_object.end()) { \
        std::string modify = flow_object[#mod_field].string_value(); \
        if (modify != "") { \
            of13::oxm_field *oxm = new of13::oxm_field(EthAddress(modify)); \
            fd.modifyField(oxm); \
        } \
    }

#define handle_modify_ip(mod_field, oxm_field) \
    if (flow_object.find(#mod_field) != flow_object.end()) { \
        std::string modify = flow_object[#mod_field].string_value(); \
        if (modify != "") { \
            of13::oxm_field *oxm = new of13::oxm_field(IPAddress(modify)); \
            fd.modifyField(oxm); \
        } \
    }

json11::Json StaticFlowPusher::handlePOST(std::vector<std::string> params, std::string body)
{
    if (params[0] == "newflow") {
        uint64_t id = std::stoull(params[1]);
        Switch* sw = sw_m->getSwitch(id);
        if (!sw) {
            return "{\"error\": \"switch now found\"}";
        }

        //getting body of request in Json format
        std::string parseMessage;
        json11::Json::object flow_object = json11::Json::parse(body, parseMessage).object_items();
        if (!parseMessage.empty()) {
            LOG(WARNING) << "Can't parse input request : " << parseMessage;
            return json11::Json("{}");
        }

        FlowDesc fd;
        handle_int(in_port);
        handle_int(out_port);
        handle_string(eth_src);
        handle_string(eth_dst);
        handle_string(ip_src);
        handle_string(ip_dst);

        handle_modify_eth(modify_eth_src, EthSrc);
        handle_modify_eth(modify_eth_dst, EthDst);
        handle_modify_ip(modify_ip_src, IPv4Src);
        handle_modify_ip(modify_ip_dst, IPv4Dst);

        sendToSwitch(sw, &fd);
        return "{\"static-flow-pusher\": \"new flow added\"}";
    }

    return "{}";
}
