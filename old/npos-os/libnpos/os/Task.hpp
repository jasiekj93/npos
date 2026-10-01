#pragma once

/**
 * @file Task.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <etl/priority_queue.h>

#include <libnpos/os/Message.hpp>
#include <libnpos/os/Bus.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os
{
    class Task
    {
    public:
        using Priority = uint8_t;

        Task(Priority priority) 
            : priority(priority) {}

        virtual ~Task() = default;

        virtual void initalize() {} 

        virtual bool isReady() const = 0;
        virtual void process()  = 0;

        inline auto getPriority() const { return priority; }

    private:
        Priority priority;
    };

    class ServiceTask  : public Task, public Service 
    {
    public:
        using MessageQueue = etl::ipriority_queue<Message*, etl::ivector<Message>, CompareMessage>;

        ServiceTask(Priority priority, MessageQueue& messageQueue, Service& service) 
            : Task(priority)
            , Service(service.getId(), service.getBus())
            , service(service)
            , messageQueue(messageQueue) {}

        virtual ~ServiceTask() = default;

        virtual void initalize() override { service.initalize(); } 

        virtual bool isReady() const override { return not messageQueue.empty(); }
        virtual void process() override
        { 
            if(messageQueue.empty())
                return;

            auto messagePtr = messageQueue.top();
            service.receive(*messagePtr);
            messageQueue.pop();
        }

        virtual bool accepts(Message::Type type) const override
        {
            return service.accepts(type);
        }

        void onReceive(Message& message) override
        {
            if(not messageQueue.full())
                messageQueue.push(&message);
            else
                ; //TODO: Handle message queue overflow
        }

        inline auto getPid() const { return service.getId(); }

    private:
        Service& service;
        MessageQueue& messageQueue;
    };


    struct CompareTask : public etl::binary_function<Task, Task, bool>
    {
        bool operator()(const Task& lhs, const Task& rhs) const
        {
            return lhs.getPriority() < rhs.getPriority();
        }
    };
}