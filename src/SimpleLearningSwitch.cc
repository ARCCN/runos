#include "SimpleLearningSwitch.hh"
#include "AppProvider.hh"
#include "Controller.hh"

REGISTER_APPLICATION(SimpleLearningSwitch, "simple-learning-switch", {"controller", ""})

SimpleLearningSwitch::SimpleLearningSwitch()
{ }
SimpleLearningSwitch::~SimpleLearningSwitch()
{ }

void SimpleLearningSwitch::init(AppProvider *provider, const Config &config)
{
    Controller* ctrl = Controller::get(provider);
    ctrl->registerHandler(this);
}

void SimpleLearningSwitch::startUp(AppProvider *provider)
{  }

std::string SimpleLearningSwitch::orderingName() const
{ return "forwarding"; }

OFMessageHandler* SimpleLearningSwitch::makeOFMessageHandler()
{ return new Handler(); }

OFMessageHandler::Action SimpleLearningSwitch::Handler::processMiss(shared_ptr<OFConnection> ofconn, Flow* flow)
{
    of13::EthSrc src;
    of13::EthDst dst;
    of13::InPort inport;

    // Learn on packet data
    flow->read(src);
    flow->read(inport);
    // Forward by packet destination
    flow->load(dst);

    if (src.value() == EthAddress("ff:ff:ff:ff:ff:ff")) {
        DLOG(WARNING) << "Broadcast source address, dropping";
        return Stop;
    }

    // TODO: how to immediately enforce this like policy?
    auto& l2table = packet_seen[ofconn->get_id()];
    l2table[src.value()] = inport.value();

    // forward
    auto it = l2table.find(dst.value());
    if (it != l2table.end()) {
        flow->forward(it->second);
        return Continue;
    } else {
        // it seems flood without worring about stp
        // TODO: change this decision after learning complete
        flow->forward(of13::OFPP_ALL);
        return Continue;
    }
}