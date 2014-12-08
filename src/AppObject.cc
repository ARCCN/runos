#include "AppObject.hh"
#include "Common.hh"

bool operator==(const AppObject& o1, const AppObject& o2)
{ return o1.id() == o2.id(); }

std::string AppObject::uint64_to_string(uint64_t dpid)
{
    std::ostringstream ss;
    ss << FORMAT_DPID << dpid;
    return ss.str();
}

time_t AppObject::connectedSince()
{ return since; }

void AppObject::connectedSince(time_t time)
{ since = time; }
