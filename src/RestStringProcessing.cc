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

#include <sstream>

#include "RestStringProcessing.hh"
#include "Controller.hh"
#include "Switch.hh"

#define SHOW(field, obj) {#field, toString_cast(obj.field())}


template<typename T>
inline std::string toString_cast(const T &value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

template<>
std::string toString_cast<std::string>(const std::string &value)
{
    return value;
}

template<>
std::string toString_cast<char>(const char &value)
{
    std::stringstream ss;
    ss << static_cast<int>(value);
    return ss.str();
}

template<>
std::string toString_cast<unsigned char>(const unsigned char &value)
{
    std::stringstream ss;
    ss << static_cast<unsigned int>(value);
    return ss.str();
}


json11::Json toJson(std::vector<of13::FlowStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        // TODO: add other fields
        ans.emplace_back(json11::Json::object {
                SHOW(length, stat),
                SHOW(table_id, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat),
                SHOW(priority, stat),
                SHOW(idle_timeout, stat),
                SHOW(hard_timeout, stat),
                {"flags", toString_cast(stat.get_flags())},
                SHOW(cookie, stat),
                SHOW(packet_count, stat),
                SHOW(byte_count, stat)
        });
    }
    return ans;
}

json11::Json toJson(std::vector<of13::PortStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                SHOW(rx_packets, stat),
                SHOW(tx_packets, stat),
                SHOW(rx_bytes, stat),
                SHOW(tx_bytes, stat),
                SHOW(rx_dropped, stat),
                SHOW(tx_dropped, stat),
                SHOW(rx_errors, stat),
                SHOW(tx_errors, stat),
                SHOW(rx_frame_err, stat),
                SHOW(rx_over_err, stat),
                SHOW(rx_crc_err, stat),
                SHOW(collisions, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat)
        });
    }
    return ans;
}

json11::Json toJson(fluid_msg::SwitchDesc &stat)
{
    return json11::Json::object {
            SHOW(mfr_desc, stat),
            SHOW(hw_desc, stat),
            SHOW(sw_desc, stat),
            SHOW(serial_num, stat),
            SHOW(dp_desc, stat)
    };
}

json11::Json toJson(of13::MultipartReplyAggregate &stat)
{
    return json11::Json::object {
            SHOW(packet_count, stat),
            SHOW(byte_count, stat),
            SHOW(flow_count, stat),
    };
}

json11::Json toJson(std::vector<of13::TableStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(active_count, stat),
                SHOW(table_id, stat),
                SHOW(lookup_count, stat),
                SHOW(matched_count, stat)
        });
    }
    return ans;
}

json11::Json toJson(std::vector<of13::Port> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                {"hw_addr", stat.hw_addr().to_string()},
                SHOW(name, stat),
                SHOW(config, stat),
                SHOW(state, stat),
                SHOW(curr, stat),
                SHOW(advertised, stat),
                SHOW(supported, stat),
                SHOW(peer, stat),
                SHOW(curr_speed, stat),
                SHOW(max_speed, stat)
        });
    }
    return ans;
}

json11::Json toJson(std::vector<of13::QueueStats> &stats)
{
    std::vector<json11::Json::object> ans;
    for (auto &stat : stats) {
        ans.emplace_back(json11::Json::object {
                SHOW(port_no, stat),
                SHOW(queue_id, stat),
                SHOW(tx_bytes, stat),
                SHOW(tx_packets, stat),
                SHOW(tx_errors, stat),
                SHOW(duration_sec, stat),
                SHOW(duration_nsec, stat),
        });
    }
    return ans;
}

json11::Json::object parse(const std::string &body)
{
    std::string forErr;
    json11::Json::object req = json11::Json::parse(body, forErr).object_items();
    if (!forErr.empty()) {
        std::ostringstream errMsg;
        errMsg << "Can't parse input request : " << forErr;
        throw errMsg.str();
    }
    return req;
}

void validateRequest(const json11::Json::object &req,
                     const std::string &mode,
                     Controller *ctrl,
                     SwitchManager *sw_m)
{
    try {
        std::stringstream errMsg;
        errMsg << "Incorrect request.";
        auto dpid = json_cast<uint64_t>(req.at("dpid"));
        if (mode == "full") {
            auto table_id = json_cast<uint8_t>(req.at("table_id"));
            if (table_id >= ctrl->handler_table()) {
                errMsg << " RestFlowMod can work only with tables in range of 0.." << ctrl->handler_table() <<
                       ". Look at RestFlowMod class definition for more details.";
                throw errMsg.str();
            }
            req.at("match");
            req.at("actions");
        }
        if (!sw_m->getSwitch(dpid)) {
            errMsg << " Switch hasn't been found";
            throw errMsg.str();
        }
    } catch (const std::string &) {
        throw;
    } catch (const std::overflow_error &errMsg) {
        throw errMsg.what();
    } catch (...) {
        throw std::string{"Incorrect request."};
    }
}