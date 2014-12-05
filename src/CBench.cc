#include "CBench.hh"
#include "Controller.hh"

REGISTER_APPLICATION(CBench, {"controller", ""})

void CBench::init(Loader* loader, const Config& config)
{
    Controller::get(loader)->registerHandler(this);
}

OFMessageHandler::Action CBench::Handler::processMiss(OFConnection* ofconn, Flow* flow)
{
    EthAddress src = flow->pkt()->readEthSrc();

    uint32_t out_port = src.get_data()[5] % 3;

    flow->setFlags(Flow::Disposable);
    flow->add_action(new of13::OutputAction(out_port, 0));

    return Stop;
}
