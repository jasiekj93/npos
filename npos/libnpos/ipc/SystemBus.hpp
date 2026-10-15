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
        static constexpr size_t MAX_PORTS = NULL_PORT;

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
            , succesor(nullptr)
        {
        }

        bool addPort(Port& port)
        {
            if(ports.full() or ports.size() >= MAX_PORTS)
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

            // One hop only, successors point at each other.
            if(succesor)
                succesor->respondToPort(message);
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
        // Messages already in the pool are used as they are; others are copied once. Null when the pool is exhausted.
        Message* allocate(const Message& message)
        {
            if(pool.is_in_pool(&message))
                return const_cast<Message*>(&message);

            if(pool.full())
                return nullptr;

            auto copy = new (pool.allocate<Message>()) Message(message);
            copy->referenceCount = 0;
            return copy;
        }

        void broadcast(Message& message)
        {
            broadcastToPorts(message);

            if(succesor)
                succesor->broadcastToPorts(message);
        }

        void broadcastToPorts(Message& message)
        {
            auto messagePtr = allocate(message);

            if(not messagePtr)
                return;

            // The bus keeps its own reference, a port may release the message from within receiveRequest.
            messagePtr->referenceCount++;

            for(auto& port : ports)
            {
                if(port->getId() != message.sender.portId and
                    port->accepts(message.type))
                {
                    messagePtr->referenceCount++;
                    port->receiveRequest(*messagePtr);
                }
            }

            release(messagePtr);
        }

        bool respondToPort(Message& message)
        {
            auto index = getIndex(message.sender.portId);

            if(index >= ports.size())
                return false;

            if(auto messagePtr = allocate(message))
            {
                messagePtr->referenceCount++;
                ports[index]->receiveResponse(*messagePtr);
            }

            return true;
        }

        void sendTo(PortId portId, Message& message)
        {
            if(sendToPort(portId, message))
                return;

            // One hop only, successors point at each other.
            if(succesor and succesor->sendToPort(portId, message))
                return;
            else
                reject(message);
        }

        bool sendToPort(PortId portId, Message& message)
        {
            auto index = getIndex(portId);

            if(index >= ports.size())
                return false;

            auto& port = *ports[index];

            if(not port.accepts(message.type))
            {
                reject(message, &port);
                return true;
            }

            if(auto messagePtr = allocate(message))
            {
                messagePtr->referenceCount++;
                port.receiveRequest(*messagePtr);
            }

            return true;
        }

        // The port is null when there is no port with the requested id.
        void reject(Message& message, Port* port = nullptr)
        {
            if(port)
                port->handleUnknownRequest(message);
            else
            {
                message.output.status = Message::STATUS_PORT_NOT_FOUND;
                respond(message);
            }
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