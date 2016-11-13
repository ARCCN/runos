/*
 * Copyright 2016 Applied Research Center for Computer Networks
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

#include "RestMultipart.hh"
#include "Controller.hh"
#include "RestListener.hh"
#include "RestStringProcessing.hh"

#include <boost/lexical_cast.hpp>
#include <algorithm>

#define SET(field, dst, src) dst.field(toString_cast<decltype(dst.field())>(src.at(#field)))

// NOTE: we use strings instead of values, because uint64_t does not fit to int -- value_type in json11

REGISTER_APPLICATION(RestMultipart, {"controller", "switch-manager", "rest-listener", ""})

void RestMultipart::init(Loader *loader, const Config &rootConfig)
{
    ctrl_ = Controller::get(loader);
    sw_m_ = SwitchManager::get(loader);
    // rest
    RestListener::get(loader)->registerRestHandler(this);
    acceptPath(Method::GET, "flow/" DPID_);
    acceptPath(Method::POST, "flow/" DPID_);
    acceptPath(Method::GET, "port/" DPID_ "/" PORTNUMBER_ALL_);
    acceptPath(Method::GET, "port-desc/" DPID_);
    acceptPath(Method::GET, "switch/" DPID_ALL_);
    acceptPath(Method::GET, "aggregate-flow/" DPID_);
    acceptPath(Method::POST, "aggregate-flow/" DPID_);
    acceptPath(Method::GET, "table/" DPID_);
    // TODO: test (mn does not support queues)
    acceptPath(Method::GET, "queue/" DPID_ "/" PORTNUMBER_ALL_ "/" QUEUEID_ALL_);

    transaction_ = ctrl_->registerStaticTransaction(this);
    connect(transaction_, &OFTransaction::response, this, &RestMultipart::onResponse);
}

json11::Json RestMultipart::handleGET(std::vector<std::string> params, std::string body)
{
    QEventLoop pause;
    connect(this, SIGNAL(ResponseHandlingFinished()), &pause, SLOT(quit()));
    try {
        if (params[0] == "flow") {
            sendGetRequest(of13::MultipartRequestFlow{}, boost::lexical_cast<uint64_t>(params[1]));
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseFlow_)}
            };
        }
        if (params[0] == "port") {
            sendGetRequest(of13::MultipartRequestPortStats{}, boost::lexical_cast<uint64_t>(params[1]), params[2]);
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responsePort_)}
            };
        }
        if (params[0] == "switch") {
            if (params[1] == "all") {
                json11::Json::array arr;
                for (const auto &sw : sw_m_->switches()) {
                    arr.emplace_back(boost::lexical_cast<std::string>(sw->id()));
                }
                return arr;
            }
            sendGetRequest(of13::MultipartRequestDesc{}, boost::lexical_cast<uint64_t>(params[1]));
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseSwitchDesc_)}
            };
        }
        if (params[0] == "aggregate-flow") {
            sendGetRequest(of13::MultipartRequestAggregate{}, boost::lexical_cast<uint64_t>(params[1]));
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseAggregate_)}
            };
        }
        if (params[0] == "table") {
            sendGetRequest(of13::MultipartRequestTable{}, boost::lexical_cast<uint64_t>(params[1]));
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseTable_)}
            };
        }
        if (params[0] == "port-desc") {
            sendGetRequest(of13::MultipartRequestPortDescription{}, boost::lexical_cast<uint64_t>(params[1]));
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responsePortDesc_)}
            };
        }
        if (params[0] == "queue") {
            sendGetRequest(of13::MultipartRequestQueue{}, boost::lexical_cast<uint64_t>(params[1]), params[2],
                           params[3]);
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseQueue_)}
            };
        }
    } catch (...) {
        return json11::Json::object{
            {"RestMultipart", "incorrect request"}
        };
    }
    return json11::Json::object{
            {"RestMultipart", "incorrect request"}
    };
}

/// implementation: tries to parse, convert and send request. If can't, returns a string with description. It's implemented by throwing std::string exception on a processing stage and catching in the body of handlePOST.
json11::Json RestMultipart::handlePOST(std::vector<std::string> params, std::string body)
{
    QEventLoop pause;
    connect(this, SIGNAL(ResponseHandlingFinished()), &pause, SLOT(quit()));

    try {
        // body parsing
        auto req = parse(body);

        if (params[0] == "flow") {
            sendPostRequest(of13::MultipartRequestFlow{}, boost::lexical_cast<uint64_t>(params[1]), req);
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseFlow_)}
            };
        }
        // todo: test. Does ovs support sending aggregate flows stats filtered by fields? Guess, no.
        if (params[0] == "aggregate-flow") {
            sendPostRequest(of13::MultipartRequestAggregate{}, boost::lexical_cast<uint64_t>(params[1]), req);
            pause.exec();
            return json11::Json::object{
                    {params[1], toJson(responseAggregate_)}
            };
        }
    } catch (const std::string &errMsg) {
        return json11::Json::object{
                {"RestMultipart", errMsg.c_str()}
        };
    } catch (...) {
        return json11::Json::object{
                {"RestMultipart", "Some error on request handling"}
        };
    }
    return json11::Json::object{
            {"RestMultipart", "incorrect request"}
    };
}

void RestMultipart::sendGetRequest(of13::MultipartRequestFlow &&req, uint64_t dpid)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    req.table_id(of13::OFPTT_ALL);
    req.out_port(of13::OFPP_ANY);
    req.out_group(of13::OFPG_ANY);
    req.cookie(0x0);  // match: cookie & mask == field.cookie & mask
    req.cookie_mask(0x0);
    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestPortStats &&req,
                                     uint64_t dpid,
                                     std::string port_number)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    try {
        req.port_no(boost::lexical_cast<uint32_t>(port_number));
    } catch (boost::bad_lexical_cast) {
        req.port_no(of13::OFPP_ANY);
    }
    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestDesc &&req, uint64_t dpid)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestAggregate &&req, uint64_t dpid)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    req.table_id(of13::OFPTT_ALL);
    // FIXME: libfluid issue: OFPP_ANY: 32 bits -> 16 bits. All bits are set to 1 => everything works
    req.out_port(of13::OFPP_ANY);
    req.out_group(of13::OFPG_ANY);
    req.cookie(0x0);
    req.cookie_mask(0x0);
    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestTable &&req, uint64_t dpid)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestPortDescription &&req, uint64_t dpid)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    req.flags(0);
    transaction_->request(sw->connection(), req);
}

void RestMultipart::sendGetRequest(of13::MultipartRequestQueue &&req,
                                     uint64_t dpid,
                                     std::string port_number,
                                     std::string queue_id)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    try {
        req.port_no(boost::lexical_cast<uint32_t>(port_number));
    } catch (const boost::bad_lexical_cast &) {
        req.port_no(of13::OFPP_ANY);
    }
    try {
        req.queue_id(boost::lexical_cast<uint32_t>(queue_id));
    } catch (const boost::bad_lexical_cast &) {
        // there is no of13::OFPQ_ALL in libfluid. Assume that "all" means setting all bits to 1.
        req.queue_id(0xFFFFFFFF);
    }
    req.flags(0);
    transaction_->request(sw->connection(), req);
}


/// converts json value to uint and adds it to the Match object
#define HANDLE_UINT(field, type) \
    try { \
        auto value = json_cast<type>(req.at(#field)); \
        match.add_oxm_field(new field{value}); \
    } catch (std::overflow_error) { \
        throw; \
    } catch (...) {}

/// exceptions are handled by caller
void RestMultipart::sendPostRequest(of13::MultipartRequestFlow &&mpReq,
                                    uint64_t dpid,
                                    json11::Json::object req)
{
    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    processInfo(mpReq, req);
    if (req.find("match") != req.end()) {
        const auto &matches = req.at("match").object_items();
        processMatches(mpReq, matches);
    }
    transaction_->request(sw_m_->getSwitch(dpid)->connection(), mpReq);
}

// note: same as for of13::MultipartRequestFlow
void RestMultipart::sendPostRequest(of13::MultipartRequestAggregate &&mpReq,
                                    uint64_t dpid,
                                    json11::Json::object req)
{

    auto sw = sw_m_->getSwitch(dpid);
    if (!sw) {
        throw "switch not found";
    }

    processInfo(mpReq, req);
    if (req.find("match") != req.end()) {
        const auto &matches = req.at("match").object_items();
        processMatches(mpReq, matches);
    }
    transaction_->request(sw->connection(), mpReq);
}

void RestMultipart::onResponse(SwitchConnectionPtr conn, std::shared_ptr<OFMsgUnion> reply)
{
    auto type = reply->base()->type();
    if (type != of13::OFPT_MULTIPART_REPLY) {
        LOG(ERROR) << "Unexpected response of type " << type  << " received, expected OFPT_MULTIPART_REPLY";
        return;
    }

    switch (reply->multipartReply.mpart_type()) {
    case of13::OFPMP_FLOW:
        responseFlow_ = reply->multipartReplyFlow.flow_stats();
        break;
    case of13::OFPMP_PORT_STATS:
        responsePort_ = reply->multipartReplyPortStats.port_stats();
        break;
    case of13::OFPMP_DESC:
        responseSwitchDesc_ = reply->multipartReplyDesc.desc();
        break;
    case of13::OFPMP_AGGREGATE:
        responseAggregate_ = reply->multipartReplyAggregate;
        break;
    case of13::OFPMP_TABLE:
        responseTable_ = reply->multipartReplyTable.table_stats();
        break;
    case of13::OFPMP_PORT_DESC:
        responsePortDesc_ = reply->multipartReplyPortDescription.ports();
        break;
    case of13::OFPMP_QUEUE:
        responseQueue_ = reply->multipartReplyQueue.queue_stats();
        break;
    default:
        LOG(ERROR) << "RestMultipart: unknown MultipartReply type with code: " << reply->multipartReply.mpart_type();
        return;
    }
    emit ResponseHandlingFinished();
}

void RestMultipart::processInfo(of13::MultipartRequestFlow &mpReq,
                                const json11::Json::object &req)
{
    auto chValue = [&] (const std::string &field, uint64_t alternative) {
        return chooseValue(req, field, uint64_t(alternative));
    };

    mpReq.table_id(chValue("table_id", of13::OFPTT_ALL));
    mpReq.out_port(chValue("out_port", of13::OFPP_ANY));
    mpReq.out_group(chValue("out_group", of13::OFPG_ANY));
    mpReq.cookie(chValue("cookie", 0x0));
    mpReq.cookie_mask(chValue("cookie_mask", 0x0));
    mpReq.flags(0);
}

void RestMultipart::processMatches(of13::MultipartRequestFlow &mpReq,
                                   const json11::Json::object &req)
{
    using namespace fluid_fix::matches;

    auto pair = req.find("in_port");
    of13::Match match;
    HANDLE_UINT(tcp_src, uint16_t)
    HANDLE_UINT(tcp_dst, uint16_t)
    HANDLE_UINT(udp_src, uint16_t)
    HANDLE_UINT(udp_dst, uint16_t)
    HANDLE_UINT(in_port, uint32_t)
    HANDLE_UINT(in_phy_port, uint32_t)
    HANDLE_UINT(metadata, uint64_t)
    HANDLE_UINT(eth_type, uint16_t)
    HANDLE_UINT(ip_proto, uint8_t)
    if ((pair = req.find("eth_src")) != req.end()) {
        match.add_oxm_field(new of13::EthSrc{pair->second.string_value()});
    }
    if ((pair = req.find("eth_dst")) != req.end()) {
        match.add_oxm_field(new of13::EthDst{pair->second.string_value()});
    }
    if ((pair = req.find("ipv4_src")) != req.end()) {
        match.add_oxm_field(new of13::IPv4Src{pair->second.string_value()});
    }
    if ((pair = req.find("ipv4_dst")) != req.end()) {
        match.add_oxm_field(new of13::IPv4Dst{pair->second.string_value()});
    }

    mpReq.match(match);
}


/// Warning: copy-paste from MultipartRequestFlow
void RestMultipart::processInfo(of13::MultipartRequestAggregate &mpReq,
                                const json11::Json::object &req)
{
    auto chValue = [&] (const std::string &field, uint64_t alternative) {
        return chooseValue(req, field, uint64_t(alternative));
    };

    mpReq.table_id(chValue("table_id", of13::OFPTT_ALL));
    mpReq.out_port(chValue("out_port", of13::OFPP_ANY));
    mpReq.out_group(chValue("out_group", of13::OFPG_ANY));
    mpReq.cookie(chValue("cookie", 0x0));
    mpReq.cookie_mask(chValue("cookie_mask", 0x0));
    mpReq.flags(0);
}

/// Warning: copy-paste from MultipartRequestFlow
void RestMultipart::processMatches(of13::MultipartRequestAggregate &mpReq,
                                   const json11::Json::object &req)
{
    using namespace fluid_fix::matches;

    auto pair = req.find("in_port");
    of13::Match match;
    HANDLE_UINT(tcp_src, uint16_t)
    HANDLE_UINT(tcp_dst, uint16_t)
    HANDLE_UINT(udp_src, uint16_t)
    HANDLE_UINT(udp_dst, uint16_t)
    HANDLE_UINT(in_port, uint32_t)
    HANDLE_UINT(in_phy_port, uint32_t)
    HANDLE_UINT(metadata, uint64_t)
    HANDLE_UINT(eth_type, uint16_t)
    HANDLE_UINT(ip_proto, uint8_t)
    if ((pair = req.find("eth_src")) != req.end()) {
        match.add_oxm_field(new of13::EthSrc{pair->second.string_value()});
    }
    if ((pair = req.find("eth_dst")) != req.end()) {
        match.add_oxm_field(new of13::EthDst{pair->second.string_value()});
    }
    if ((pair = req.find("ipv4_src")) != req.end()) {
        match.add_oxm_field(new of13::IPv4Src{pair->second.string_value()});
    }
    if ((pair = req.find("ipv4_dst")) != req.end()) {
        match.add_oxm_field(new of13::IPv4Dst{pair->second.string_value()});
    }

    mpReq.match(match);
}