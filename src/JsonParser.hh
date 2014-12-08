#pragma once

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <glog/logging.h>

class JSONparser
{
public:
    std::string src;
public:
    JSONparser();
    JSONparser(bool array);
    void addToArray(JSONparser value);
    std::string get();
    void fill(std::string JSONtext);
    void addKey(std::string key);
    void addKey(std::string key, std::string baseKey);
    void addValue(std::string baseKey, std::string value);
    void addValue(std::string baseKey, JSONparser value);
    void addValue(std::string baseKey, int value);
    void addValue(std::string baseKey, uint32_t value);
    void addValue(std::string baseKey, uint64_t value);
    void addValue(std::string baseKey, bool value);
    void addValue(std::string baseKey, const char* value);
    std::string getValue(std::string baseKey);
    std::string getValue(int num);
};
