#pragma once

#include <linux/types.h>
#include <string>
#include <time.h>

#include "JsonParser.hh"

/**
* This abstract class is used in applications.
* Objects in your app must inherit this class if app uses event model
*/
class AppObject
{
    /// When object was created
    time_t since;
public:
    AppObject(): since(0) {}

    /**
     * You must define JSON representation for your object
     */
    virtual JSONparser formJSON() = 0;

    /**
     * 64-bit unique identifier for your object
     */
    virtual uint64_t id() const = 0;

    /**
     * Object's created time getter and setter
     */
    time_t connectedSince();
    void connectedSince(time_t time);

    /**
     * Translate 64-bit identifier (DPID in switches) to string format
     */
    static std::string uint64_to_string(uint64_t dpid);

    /**
     * Define the equality operator between objects
     */
    friend bool operator==(const AppObject& o1, const AppObject& o2);
};
