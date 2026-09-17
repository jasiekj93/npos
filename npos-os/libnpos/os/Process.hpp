#pragma once

/**
 * @file Process.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <etl/queue.h>

#include <libnpos/ipc/MessagePacket.hpp>
#include <libnpos/nps/Task.hpp>
#include <libnpos/os/Service.hpp>
#include <libnpos/os/message/Error.hpp>

namespace npos::os
{
    class Process : public ipc::MessageRouter, public nps::Task
    {
    public:
        Process(nps::Task::Priority priority)
            : ipc::MessageRouter(0)
            , nps::Task(priority)
        {
        }

        virtual ~Process() = default;

        // void onReceive(const Message&) override;
        // virtual bool accepts(Message::Id) const override;

        // void initalize() override;
        // virtual bool isReady() const override;
        // virtual void process() override;
    };

    template <typename... TMessageTypes>
    class QueuedProcess : public ipc::MessageRouter, public nps::Task
    {
    public:
        using MessagePacket = ipc::MessagePacket<TMessageTypes...>;
        using MessageQueue = etl::iqueue<MessagePacket>;

        QueuedProcess(nps::Task::Priority priority, message::Bus& bus, MessageQueue& messageQueue, Service& service)
            : ipc::MessageRouter(0)
            , nps::Task(priority) 
            , bus(bus)
            , messageQueue(messageQueue)
            , service(service)
        {
        }

        void onReceive(const ipc::Message& message) override
        {
            if(messageQueue.full())
                //chociaż tu bardziej syslog? czy syslog to osobny proces?
                bus.receive(message::Error(message::Error::MESSAGE_QUEUE_FULL));
            else
                messageQueue.emplace(MessagePacket(message));
        }

        bool accepts(ipc::Message::Id id) const override
        {
            return MessagePacket::accepts(id);
        }

        void initalize() override
        {
            service.initalize();
        }

        bool isReady() const override
        {
            return (not messageQueue.empty());
        }

        void process() override
        {
            auto& packet = messageQueue.front();
            auto& message = packet.get();
            service.onReceive(message);
            messageQueue.pop();
        }

    private:
        message::Bus& bus;
        MessageQueue& messageQueue;
        Service& service;
    };
}