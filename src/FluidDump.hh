#pragma once

#include "Common.hh"
#include <string>

std::string dump(OFMsg* msg);
std::string dump(of13::OXMTLV* tlv);
std::string dump(Action* tlv);
std::string dump(of13::Instruction* tlv);
std::string dump(of13::oxm_ofb_match_fields field);
