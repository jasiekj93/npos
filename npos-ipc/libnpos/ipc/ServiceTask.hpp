#pragma once

/**
 * @file ServiceTask.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/queue.h>
#include <etl/vector.h>

#include <libnpos/nps/Task.hpp>
#include <libnpos/ipc/Service.hpp>
#include <libnpos/ipc/Port.hpp>
#include <libnpos/ipc/Bus.hpp>

namespace npos::ipc
{
    class ServiceTask : public npos::nps::Task, public Port, public Service::Api
    {
    public:
        using ServiceListInt = etl::ivector<Service*>;
        using MessageQueueInt = etl::iqueue<Message*>;

        template<size_t N>
        using MessageQueue = etl::queue<Message*, N>;

        template<size_t N>
        using ServiceList = etl::vector<Service*, N>;

        ServiceTask(Priority priority, ServiceListInt& services, MessageQueueInt& messageQueue, Bus& bus)
            : nps::Task(priority)
            , services(services)
            , messageQueue(messageQueue)
            , bus(bus)
        {
        }

        bool subscribe(Service& service)
        {
            if(services.full())
                return false;
            else
            {
                auto id = services.size();
                services.push_back(&service);
                service.setId(id);
                return true;
            }
        }

        //Port
        bool accepts(Message::Type type) const override
        {
            for (auto& service : services)
            {
                if (service->accepts(type))
                    return true;
            }

            return false;
        }

        void receive(const Message& message) override
        {
            if(messageQueue.full())
                return; //TODO handle
            else
                messageQueue.push(const_cast<Message*>(&message));
        }

        //nps::Task
        virtual void initialize() override
        {
            for (auto& service : services)
                service->initialize();
        }

        virtual bool isReady() const override
        {
            return not messageQueue.empty();
        }

        virtual void process()
        {
            if(messageQueue.empty())
                return; 

            auto message = messageQueue.front();
            dispatch(*message);
            messageQueue.pop();
            bus.release(message);
        }

        //Service::Api
        void send(Message& message) override
        {
            message.sender = getId();

            if(message.recipient.portId == getId())
                dispatch(message);
            else
                bus.send(message);
        }

        void respond(Message& message) override
        {
            if(message.sender == getId())
                dispatch(message);
            else
                bus.respond(message);
        }

        PortId getPortId() const override
        {
            return getId();
        }
    
    protected:
        void dispatch(Message& message)
        {
            if(message.recipient.serviceId != NULL_SERVICE)
            {
                if(message.recipient.serviceId < services.size())
                    services[message.recipient.serviceId]->handle(message);
            }
            else
            {
                for (auto& service : services)
                {
                    if (service->accepts(message.type))
                        service->handle(message);
                }
            }
        }

    private:
        ServiceListInt& services;
        MessageQueueInt& messageQueue;
        Bus& bus;
    };
}