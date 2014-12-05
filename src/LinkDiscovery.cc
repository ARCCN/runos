// TODO: security: add cookie to LLDP messages
// TODO: test on unstable links (broken by timeout)

#include <tins/macros.h>
#include <fluid/util/util.h>
#include "LLDP.hh"
#include "Controller.hh"
#include "LinkDiscovery.hh"

REGISTER_APPLICATION(LinkDiscovery, {"switch-manager", "controller", ""})

void LinkDiscovery::init(Loader *loader, const Config &rootConfig)
{
    qRegisterMetaType<switch_and_port>();

    /* Initialize members */
    m_timer = new QTimer(this);

    /* Read configuration */
    auto config = config_cd(rootConfig, "link-discovery");
    c_poll_interval = config_get(config, "poll-interval", 120);

    /* Get dependencies */
    Controller*    ctrl  = Controller::get(loader);
    m_switch_manager     = SwitchManager::get(loader);

    /* Connect with other applications */
    ctrl->registerHandler(this);

    QObject::connect(m_switch_manager, &SwitchManager::switchDiscovered,
         [this](Switch* dp) {
             QObject::connect(dp, &Switch::portUp, this, &LinkDiscovery::portUp);
             QObject::connect(dp, &Switch::portDown, this, &LinkDiscovery::portDown);
             QObject::connect(dp, &Switch::portModified, this, &LinkDiscovery::portModified);
         });

    QObject::connect(this, &LinkDiscovery::lldpReceived,
                     this, &LinkDiscovery::onLldpReceived,
                     Qt::QueuedConnection);

    connect(m_timer, SIGNAL(timeout()), this, SLOT(pollTimeout()));

    /* Do logging */
    QObject::connect(this, &LinkDiscovery::linkDiscovered,
         [](switch_and_port from, switch_and_port to) {
             LOG(INFO) << "Link discovered: "
                     << FORMAT_DPID << from.dpid << ':' << from.port << " -> "
                     << FORMAT_DPID << to.dpid << ':' << to.port;
         });
    QObject::connect(this, &LinkDiscovery::linkBroken,
         [](switch_and_port from, switch_and_port to) {
             LOG(INFO) << "Link broken: "
                     << FORMAT_DPID << from.dpid << ':' << from.port << " -> "
                     << FORMAT_DPID << to.dpid << ':' << to.port;

        });
}

void LinkDiscovery::startUp(Loader *)
{
    // Start LLDP polling
    m_timer->start(c_poll_interval * 1000);
}

void LinkDiscovery::portUp(Switch *dp, of13::Port port)
{
    // Don't discover reserved ports
    if (port.port_no() > of13::OFPP_MAX)
        return;

    if (!(port.state() & of13::OFPPS_LINK_DOWN)) {
        // Send first packet immediately
        sendLLDP(dp, port);
    }
}

void LinkDiscovery::portModified(Switch* dp, of13::Port port, of13::Port old_port)
{
    bool live = !(port.state() & of13::OFPPS_LINK_DOWN);
    bool old_live = !(old_port.state() & of13::OFPPS_LINK_DOWN);

    if (live && !old_live) {
        // Send first packet immediately
        sendLLDP(dp, port);
    } else if (!live && old_live) {
        // Remove discovered link if it exists
        clearLinkAt(switch_and_port{dp->id(), port.port_no()});
    }
}

void LinkDiscovery::portDown(Switch *dp, uint32_t port_no)
{
    clearLinkAt(switch_and_port{dp->id(), port_no});
}

TINS_BEGIN_PACK
struct lldp_packet {
    uint64_t dst_mac:48;
    uint64_t src_mac:48;
    uint16_t eth_type;

    uint16_t chassis_id_header;
    uint8_t chassis_id_sub;
    uint64_t chassis_id_sub_mac:48;

    uint16_t port_id_header;
    uint8_t port_id_sub;
    uint32_t port_id_sub_component;

    uint16_t ttl_header;
    uint16_t ttl_seconds;

    uint16_t dpid_header;
    uint32_t dpid_oui:24;
    uint8_t  dpid_sub;
    uint64_t dpid_data;

    uint16_t end;
} TINS_END_PACK;

void LinkDiscovery::sendLLDP(Switch *dp, of13::Port port)
{
    lldp_packet lldp;
    lldp.dst_mac = hton64(0x0180c200000eULL) >> 16ULL;
    uint8_t* mac = port.hw_addr().get_data();
    lldp.src_mac = hton64(((uint64_t) mac[0] << 40ULL) |
                         ((uint64_t) mac[1] << 32ULL) |
                         ((uint64_t) mac[2] << 24ULL) |
                         ((uint64_t) mac[3] << 16ULL) |
                         ((uint64_t) mac[4] << 8ULL) |
                         (uint64_t) mac[5] ) >> 16ULL;
    lldp.eth_type = hton16(LLDP_ETH_TYPE);

    lldp.chassis_id_header = lldp_tlv_header(LLDP_CHASSIS_ID_TLV, 7);
    lldp.chassis_id_sub    = LLDP_CHASSIS_ID_SUB_MAC;
    // lower 48 bits of datapath id represents switch MAC address
    lldp.chassis_id_sub_mac = hton64(dp->id()) >> 16;

    lldp.port_id_header = lldp_tlv_header(LLDP_PORT_ID_TLV, 5);
    lldp.port_id_sub    = LLDP_PORT_ID_SUB_COMPONENT;
    lldp.port_id_sub_component = hton32(port.port_no());

    lldp.ttl_header = lldp_tlv_header(LLDP_TTL_TLV, 2);
    lldp.ttl_seconds = hton16(c_poll_interval);

    lldp.dpid_header = lldp_tlv_header(127, 12);
    lldp.dpid_oui    = hton32(0x0026e1) >> 8; // OpenFlow
    lldp.dpid_sub    = 0;
    lldp.dpid_data   = hton64(dp->id());

    lldp.end = 0;

    VLOG(5) << "Sending LLDP packet to " << port.name();
    of13::PacketOut po;
    of13::OutputAction action(port.port_no(), of13::OFPCML_NO_BUFFER);
    po.buffer_id(OFP_NO_BUFFER);
    po.data(&lldp, sizeof lldp);
    po.add_action(action);

    // Send packet 3 times to prevent drops
    dp->send(&po);
    dp->send(&po);
    dp->send(&po);
}

OFMessageHandler::Action LinkDiscovery::Handler::processMiss(OFConnection *ofconn, Flow *flow)
{
    if (flow->match(of13::EthType(LLDP_ETH_TYPE))) {
        Switch* sw = app->m_switch_manager->getSwitch(ofconn);
        lldp_packet lldp;

        flow->idleTimeout(0);
        flow->timeToLive(0);
        flow->setFlags(Flow::Disposable);

        auto pkt = flow->pkt()->serialize();
        if (pkt.size() < sizeof lldp) {
            LOG(ERROR) << "LLDP packet received is too small";
            return Stop;
        }
        memcpy(&lldp, pkt.data(), sizeof lldp);

        switch_and_port source;
        switch_and_port target;
        source.dpid = ntoh64(lldp.dpid_data);
        source.port = ntoh32(lldp.port_id_sub_component);
        target.dpid = sw->id();
        target.port = flow->pkt()->readInPort();
        DVLOG(5) << "LLDP packet received on " << sw->port(target.port).name();

        if (!(source < target))
            std::swap(source, target);
        emit app->lldpReceived(source, target);

        return Stop;
    } else {
        return Continue;
    }
}

Link::Link(switch_and_port const &source_, switch_and_port const &target_, valid_through_t const & valid_through_)
    : source(source_), target(target_), valid_through(valid_through_)
{
    if (!(source < target))
        std::swap(source, target);
}

void LinkDiscovery::onLldpReceived(switch_and_port from, switch_and_port to)
{
    Link link(from, to, std::chrono::steady_clock::now() +
                        std::chrono::seconds(c_poll_interval * 2));
    bool isNew = true;

    // Remove older edge
    auto out_it = m_out_edges.find(from);
    if (out_it != m_out_edges.end()) {
        m_links.erase(out_it->second);
        m_out_edges.erase(from);
        m_out_edges.erase(to);
        isNew = false;
    }

    // Insert new
    auto it = m_links.insert(link).first;
    m_out_edges[from] = it;
    m_out_edges[to] = it;

    if (isNew) {
        emit linkDiscovered(from, to);
    }
}

void LinkDiscovery::clearLinkAt(const switch_and_port &source)
{
    VLOG(5) << "clearLinkAt " << FORMAT_DPID << source.dpid << ':' << source.port;

    switch_and_port target;
    auto out_edges_it = m_out_edges.find(source);
    if (out_edges_it == m_out_edges.end())
        return;
    auto edge_it = out_edges_it->second;

    if (edge_it->source == source) {
        target = edge_it->target;
    } else {
        assert(edge_it->target == source);
        target = edge_it->source;
    }

    m_links.erase(edge_it);
    assert(m_out_edges.erase(source) == 1);
    assert(m_out_edges.erase(target) == 1);

    if (source < target)
        emit linkBroken(source, target);
    else
        emit linkBroken(target, source);
}

void LinkDiscovery::pollTimeout()
{
    auto now = std::chrono::steady_clock::now();

    // Remove all expired links
    while (!m_links.empty() && m_links.begin()->valid_through < now) {
        auto top = m_links.begin();
        assert(m_out_edges.erase(top->source) == 1);
        assert(m_out_edges.erase(top->target) == 1);
        emit linkBroken(top->source, top->target);
        m_links.erase(top);
    }

    // Send LLDP packets to all known links
    for (Switch* sw : m_switch_manager->switches()) {
        for (of13::Port &port : sw->ports()) {
            if (port.port_no() > of13::OFPP_MAX)
                continue;
            sendLLDP(sw, port);
        }
    }
}

OFMessageHandler *LinkDiscovery::makeOFMessageHandler()
{
    return new Handler(this);
}

std::string LinkDiscovery::orderingName() const
{
    return "link-discovery";
}

bool LinkDiscovery::isPostreq(const std::string &name) const
{
    return (name == "forwarding");
}

