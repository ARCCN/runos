#include "SimpleLearningSwitch.hh"
#include "Controller.hh"

REGISTER_APPLICATION(SimpleLearningSwitch, {"controller", ""})

void SimpleLearningSwitch::init(Loader *loader, const Config &config)
{
    Controller* ctrl = Controller::get(loader);
    ctrl->registerHandler(this);
}

std::string SimpleLearningSwitch::orderingName() const
{ return "forwarding"; }

OFMessageHandler* SimpleLearningSwitch::makeOFMessageHandler()
{ return new Handler(); }

OFMessageHandler::Action SimpleLearningSwitch::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    static EthAddress broadcast("ff:ff:ff:ff:ff:ff");

    // Learn on packet data
    EthAddress eth_src = flow->loadEthSrc();
    uint32_t   in_port = flow->pkt()->readInPort();
    // Forward by packet destination
    EthAddress eth_dst = flow->loadEthDst();

    if (eth_src == broadcast) {
        DLOG(WARNING) << "Broadcast source address, dropping";
        return Stop;
    }

    seen_port[eth_src] = in_port;

    // forward
    auto it = seen_port.find(eth_dst);

    if (it != seen_port.end()) {
        flow->idleTimeout(60);
        flow->timeToLive(5 * 60);
        flow->add_action(new of13::OutputAction(it->second, 0));
        return Continue;
    } else {
        LOG(INFO) << "Flooding for unknown address " << eth_dst.to_string();

        if (eth_dst == broadcast) {
            flow->idleTimeout(0);
            flow->timeToLive(0);
        } else {
            flow->setFlags(Flow::Disposable);
        }

        // Should be replaced with STP ports
        flow->add_action(new of13::OutputAction(of13::OFPP_ALL, 128));
        return Continue;
    }
}
