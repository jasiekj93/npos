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

        inline void setSuccesor(SystemBus& succ)
        {
            succesor = &succ;
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
            if(respondToPort(message))
                return;
            else if(succesor)
                succesor->respondToPort(message);
        }


    protected:
        void broadcast(Message& message)
        {
            broadcastToPorts(message);

            if(succesor)
                succesor->broadcastToPorts(message);
        }

        void broadcastToPorts(Message& message)
        {
            for(auto& port : ports)
            {
                if(port->getId() != message.sender and
                    port->accepts(message.type))
                {
                    port->receive(message);
                }
            }
        }

        bool respondToPort(Message& message)
        {
            auto index = getIndex(message.sender);

            if(index < ports.size())
            {
                ports[index]->receive(message);
                return true;
            }
            return false;
        }

        void sendTo(PortId portId, Message& message)
        {
            auto index = getIndex(portId);

            if(index < ports.size())
                ports[index]->receive(message);
            else if(succesor)
                succesor->send(message);
        }

        size_t getIndex(PortId id) const
        {
            auto index = static_cast<size_t>(id) - static_cast<size_t>(initialPort);
            return index;
        }

    private:
        PortList& ports;
        PortId initialPort;
        SystemBus* succesor;
    };
}