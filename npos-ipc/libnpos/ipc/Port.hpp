#pragma once

/**
 * @file Port.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc
{
    class Port
    {
    public:
        Port() = default;

        virtual bool accepts(Message::Type) const = 0;
        virtual void receive(const Message&) = 0;

        inline void setId(PortId newId) { id = newId; }
        inline PortId getId() const { return id; }

    private:
        PortId id = 0;
    };
}
