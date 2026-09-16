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

namespace npos::os
{
    template <typename... TMessageTypes>
    class Process : public ipc::MessageRouter, public nps::Task
    {
    public:
        using MessagePacket = ipc::MessagePacket<TMessageTypes...>;
        using MessageQueue = etl::iqueue<MessagePacket>;

        Process(nps::Task::Priority priority, MessageQueue& messageQueue, Service& service)
            : ipc::MessageRouter(0)
            , nps::Task(priority) 
            , messageQueue(messageQueue)
            , service(service)
        {
        }

        void onReceive(const Message& message) override
        {
            //TODO obsługa przerwań? Czy to bardziej user space?
            if(messageQueue.full())
            {
                // Handle full message queue scenario
            }
            else
                messageQueue.emplace(MessagePacket(message));
        }

        bool accepts(Message::Id id) const override
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