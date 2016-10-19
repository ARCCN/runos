#pragma once

#include "SwitchConnectionFwd.hh"

#include <cstdint>
#include <cstddef>

#include <QMetaType>

/** @file */

namespace fluid_base {
class OFConnection;
}

namespace fluid_msg {
class OFMsg;
}

namespace runos {

/**
 * Connection with physical switch for OpenFlow communication
 */
class SwitchConnection {
    const uint64_t m_dpid;

public:
	/** get dpid of switch, which connected */
    uint64_t dpid() const
    { return m_dpid; }

    /** Connection alive or not */
    bool alive() const;

    uint8_t version() const;

    /**
     * Send OpenFlow message to switch
     *
     * @param msg message.
     */
    void send(const fluid_msg::OFMsg& msg);

    void close();

protected:
    fluid_base::OFConnection* m_ofconn;
    SwitchConnection(fluid_base::OFConnection* ofconn, uint64_t dpid);
};

} // namespace runos

Q_DECLARE_METATYPE(runos::SwitchConnectionPtr);
