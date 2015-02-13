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

#include "JsonParser.hh"

JSONparser::JSONparser()
{
    src = "{ }";
}

JSONparser::JSONparser(bool array)
{
    if (array)
        src = "[]";
    else
        src = "{ }";
}

void JSONparser::addToArray(JSONparser value)
{
    int pos = src.find_last_of("]");
    if (pos != 1) {
        src.insert(pos, ", ");
    }
    src.insert(src.find_last_of("]"), value.get());
}

std::string JSONparser::get()
{
    std::string str = src;
    uint from = 0;

    while (from < str.length()) {
        int open = str.find('[', from);
        int close = str.find(']', open);
        int space = str.find(' ', open);

        if (open <= 0) break;

        if ((space > 0 && space < close) || close == open + 1) {
            from = close + 1;
            continue;
        }
        else {
            str.erase(str.begin() + close);
            str.erase(str.begin() + open);
        }
    }
    return str;
}

void JSONparser::fill(std::string JSONtext)
{
    src = JSONtext;
}

void JSONparser::addKey(std::string key)
{
    int pos = src.find_last_of("}");
    if (src.compare("{ }") == 0 ) {
        src = "{ \"" + key + "\": [] }";
    }
    else {
        src = src.insert(pos-1, ", \"" + key + "\": []");
    }
}

void JSONparser::addKey(std::string key, std::string baseKey)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    if (src.substr(pos + len + 4, 1) == "]")
        src.insert(pos + len + 4, "{ \"" + key + "\": [] }");
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        src.insert(pos + len + 3 + pos2, ", { \"" + key + "\": [] }");
    }
}

void JSONparser::addValue(std::string baseKey, std::string value)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    if (src.substr(pos + len + 4, 1) == "]")
        src.insert(pos + len + 4, "\"" + value + "\"");
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        src.insert(pos + len + 3 + pos2, ", \"" + value + "\"");
    }
}

void JSONparser::addValue(std::string baseKey, JSONparser value)
{
    int counter = 0;
    int pos = src.find(baseKey), pos2 = 0;
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    if (src.substr(pos + len + 4, 1) == "]") {
        src.insert(pos + len + 4, value.get());
    }
    else {
        std::string tmp = src.substr(pos + len + 4);
        while (tmp.find('[') > 0 || counter != 0) {
            if (tmp.find('[') < tmp.find(']')) {
                counter++;
                int tmp_pos = tmp.find('[');
                tmp = tmp.substr(tmp_pos + 1);
                pos2 += tmp_pos + 1;
            }
            else {
                if (counter == 0)
                    break;

                counter--;
                int tmp_pos = tmp.find(']');
                tmp = tmp.substr(tmp_pos + 1);
                pos2 += tmp_pos + 1;
            }
        }
        pos2 += tmp.find("]");
        src.insert(pos + len + 4 + pos2, ", " + value.get());
    }
}

void JSONparser::addValue(std::string baseKey, int value)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    std::ostringstream convert;
    convert << value;
    if (src.substr(pos + len + 4, 1) == "]") {
        src.insert(pos + len + 4, convert.str());
    }
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        src.insert(pos + len + 3 + pos2, ", " + convert.str());
    }
}

void JSONparser::addValue(std::string baseKey, uint32_t value)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    std::ostringstream convert;
    convert << value;
    if (src.substr(pos + len + 4, 1) == "]") {
        src.insert(pos + len + 4, convert.str());
    }
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        src.insert(pos + len + 3 + pos2, ", " + convert.str());
    }
}

void JSONparser::addValue(std::string baseKey, uint64_t value)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    std::ostringstream convert;
    convert << value;
    if (src.substr(pos + len + 4, 1) == "]") {
        src.insert(pos + len + 4, convert.str());
    }
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        src.insert(pos + len + 3 + pos2, ", " + convert.str());
    }
}

void JSONparser::addValue(std::string baseKey, bool value)
{
    int pos = src.find(baseKey);
    int len = baseKey.length();
    if (pos <= 0) {
        addKey(baseKey);
        pos = src.find(baseKey);
    }
    if (src.substr(pos + len + 4, 1) == "]") {
        if (value)
            src.insert(pos + len + 4, "true");
        else
            src.insert(pos + len + 4, "false");
    }
    else {
        std::string tmp = src.substr(pos + len + 3);
        int pos2 = tmp.find("]");
        if (value)
            src.insert(pos + len + 3 + pos2, ", true");
        else
            src.insert(pos + len + 3 + pos2, ", false");
    }
}

void JSONparser::addValue(std::string baseKey, const char *value)
{
    std::string str = value;
    addValue(baseKey, str);
}

std::string JSONparser::getValue(std::string baseKey)
{
    uint lfpos, rfpos, pos = src.find(baseKey);
    std::string out;

    if (pos == std::string::npos) {
        LOG(WARNING) << "Key wasn't found! Key: " << baseKey;
        return "-1";
    }

    if (src[pos + baseKey.length() + 1] == ':') {
        lfpos = pos + baseKey.length() + 2;
        if (src[lfpos] == ' ') ++lfpos;
        rfpos = src.find(',', lfpos);
        if (rfpos <= 0)
            rfpos = src.find('}', lfpos);
        else {
            if (rfpos > src.find('}', lfpos))
                rfpos = src.find('}', lfpos);
        }
        out = src.substr(lfpos, rfpos - lfpos);
        while (out.find('"') != std::string::npos)
            out.erase(out.begin() + out.find('"'));
        return out;
    }
    else {
        LOG(WARNING) << "Incorrect JSON format: " << src;
        return "JSON::NON_VALUE";
    }
}

std::string JSONparser::getValue(int num)
{
    int now = 0;
    std::string str = src, out;

    while (now < num) {
        int pos = str.find(':');
        if (pos <= 0) {
            return "ERROR";
        }
        str = str.substr(pos + 1);
        ++now;
    }
    int pos = str.find(':');
    if (pos <= 0) {
        return "ERROR";
    }
    str = str.substr(pos + 1);

    if (str[0] == '['){
        str = str.substr(1);
        while (str[0] != ']') {
            out.push_back(str[0]);
            str = str.substr(1);
        }
        return out;
    }

    while (str[0] != ',' && str[0] != ' ' && str[0] != '}') {
        out.push_back(str[0]);
        str = str.substr(1);
    }

    while (out.find('"') != std::string::npos)
        out.erase(out.begin() + out.find('"'));

    return out;
}
