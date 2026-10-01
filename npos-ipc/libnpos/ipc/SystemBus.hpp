#pragma once

/**
 * @file SystemBus.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/vector.h>
#include <etl/pool.h>

#include <libnpos/ipc/Port.hpp>
#include <libnpos/ipc/Bus.hpp>

namespace npos::ipc
{
    class SystemBus : public Bus
    {
    public:
        using PortListInt = etl::ivector<Port*>;
        using PoolInt = etl::ipool;
        
        template <size_t N>
        using Pool = etl::pool<Message, N>;

        template <size_t N>
        using PortList = etl::vector<Port*, N>;

        SystemBus(PortListInt& ports, PoolInt& pool, PortId initialPort)
            : ports(ports)
            , pool(pool)
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

        void send(Message& message) override
        {
            if(message.recipient.portId != NULL_PORT)
                sendTo(message.recipient.portId, message);
            else
                broadcast(message);
        }

        void respond(Message& message) override
        {
            if(respondToPort(message))
                return;
            else if(succesor)
                succesor->respondToPort(message, true);
        }

        void release(Message* messagePtr) override
        {
            if(not messagePtr or not pool.is_in_pool(messagePtr))
                return;

            if(messagePtr->referenceCount > 0)
                messagePtr->referenceCount--;

            if(messagePtr->referenceCount == 0)
                return pool.release(const_cast<Message*>(messagePtr));

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
            auto messagePtr = &message;

            if(not pool.is_in_pool(&message))
                messagePtr = new (pool.allocate<Message>()) Message(message);

            for(auto& port : ports)
            {
                if(port->getId() != message.sender and
                    port->accepts(message.type))
                {
                    messagePtr->referenceCount++;
                    port->receive(*messagePtr);
                }
            }

            if(messagePtr->referenceCount == 0)
                if(pool.is_in_pool(messagePtr))
                    pool.release(messagePtr);
        }

        bool respondToPort(Message& message, bool fromSuccesor = false)
        {
            auto index = getIndex(message.sender);

            if(index < ports.size())
            {
                auto messagePtr = &message;

                if(not pool.is_in_pool(&message))
                    messagePtr = new (pool.allocate<Message>()) Message(message);

                if(not fromSuccesor)
                    messagePtr->referenceCount++;
                ports[index]->receive(*messagePtr);
                return true;
            }
            return false;
        }

        void sendTo(PortId portId, Message& message)
        {
            auto index = getIndex(portId);

            if(index < ports.size())
            {
                auto messagePtr = &message;
                if(not pool.is_in_pool(&message))
                    messagePtr = new (pool.allocate<Message>()) Message(message);
                
                messagePtr->referenceCount++;
                ports[index]->receive(*messagePtr);
            }

            else if(succesor)
                succesor->send(message);
        }

        size_t getIndex(PortId id) const
        {
            auto index = static_cast<size_t>(id) - static_cast<size_t>(initialPort);
            return index;
        }

    private:
        PortListInt& ports;
        PoolInt& pool;
        PortId initialPort;
        SystemBus* succesor;
    };
}