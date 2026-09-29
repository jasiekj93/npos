#pragma once

/**
 * @file SystemBus.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/Port.hpp>
#include <libnpos/ipc/Bus.hpp>

namespace npos::ipc
{
    class SystemBus : public Bus
    {
    public:
        using PortList = etl::ivector<Port*>;

        SystemBus(PortList& ports, PortId initialPort)
            : ports(ports)
            , initialPort(initialPort)
        {
        }

        bool addPort(Port& port)
        {
            if(ports.full())
                return false;

            auto portId = ports.size() + initialPort;
            port.setId(portId);
            ports.push_back(&port);
            return true;
        }

        inline void setSuccessor(SystemBus* succ)
        {
            successor = succ;
        }

        void send(Message& message)
        {
            if(message.recipient.portId != NULL_PORT)
                sendTo(message.recipient.portId, message);
            else
                broadcast(message);
        }

        void respond(Message& message)
        {
            sendTo(message.sender, message);
        }


    protected:
        void broadcast(Message& message)
        {
            for(auto& port : ports)
            {
                if(port->getId() != message.sender and
                    port->accepts(message.type))
                {
                    port->receive(message);
                }
            }

            if(successor)
                successor->send(message);
        }

        void sendTo(PortId portId, Message& message)
        {
            auto index = getIndex(portId);

            if(index < ports.size())
                ports[index]->receive(message);
            else if(successor)
                successor->send(message);
        }

        size_t getIndex(PortId id) const
        {
            auto index = static_cast<size_t>(id) - static_cast<size_t>(initialPort);
            return index;
        }

    private:
        PortList& ports;
        PortId initialPort;
        SystemBus* successor;
    };
}