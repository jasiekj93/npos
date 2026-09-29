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
        using ServiceList = etl::ivector<Service*>;
        using MessageQueue = etl::iqueue<Message>;

        ServiceTask(Priority priority, ServiceList& services, MessageQueue& messageQueue, Bus& bus)
            : nps::Task(priority)
            , services(services)
            , messageQueue(messageQueue)
            , bus(bus)
            , port(NULL_PORT)
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
                messageQueue.push(message);
        }

        //nps::Task
        virtual bool isReady() const override
        {
            return not messageQueue.empty();
        }

        virtual void process()
        {
            if(messageQueue.empty())
                return; 

            auto message = messageQueue.front();
            dispatch(message);
            messageQueue.pop();
        }

        //Service::Api
        void send(Message& message) override
        {
            message.sender = port;

            if(message.recipient.portId == port)
                dispatch(message);
            else
                bus.send(message);
        }

        void respond(Message& message) override
        {
            if(message.sender == port)
                dispatch(message);
            else
                bus.respond(message);
        }

        PortId getPortId() const override
        {
            return port;
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
        ServiceList& services;
        MessageQueue& messageQueue;
        Bus& bus;
        PortId port;
    };
}