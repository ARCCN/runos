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
