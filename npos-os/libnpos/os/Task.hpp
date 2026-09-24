#pragma once

/**
 * @file Task.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/nps/Task.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os
{
    class Task : public nps::Task, public os::Service
    {
    public:
        using Tid = uint8_t;
        using Priority = nps::Task::Priority;
        using SystemBus = ipc::MessageRouter;

        Task(Priority priority, Tid tid, SystemBus& bus) 
            : nps::Task(priority)
            , os::Service(bus)
            , tid(tid) {}

        virtual ~Task() = default;

        // virtual void onReceive(const Message&) = 0;
        // virtual bool accepts(Message::Id) const = 0;

        // virtual bool isReady() const = 0;
        // virtual void process() = 0;

        inline Tid getTid() const { return tid; }

    private:
        Tid tid;
    };

    template <typename... TMessageTypes>
    class ServiceTask : public Task
    {
    public:
        using MessagePacket = ipc::MessagePacket<TMessageTypes...>;
        using MessageQueue = etl::iqueue<MessagePacket>;

        ServiceTask(Priority priority, Tid tid, SystemBus& bus, Service& service, MessageQueue& messageQueue)
            : Task(priority, tid, bus)
            , messageQueue(messageQueue)
            , service(service)
        {
        }

        void onReceive(const ipc::Message& message) override
        {
            if(messageQueue.full())
                //chociaż tu bardziej syslog? czy syslog to osobny proces?
                broadcast(message::Error(message::Error::MESSAGE_QUEUE_FULL));
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
        MessageQueue& messageQueue;
        Service& service;
    };
}