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
        static constexpr size_t MAX_SERVICES = NULL_SERVICE;

        using ServiceListInt = etl::ivector<Service*>;
        using RequestQueueInt = etl::iqueue<Message*>;
        using ResponseQueueInt = etl::iqueue<Message*>;

        template<size_t N>
        using RequestQueue = etl::queue<Message*, N>;

        template<size_t N>
        using ResponseQueue = etl::queue<Message*, N>;

        template<size_t N>
        using ServiceList = etl::vector<Service*, N>;

        ServiceTask(Priority priority, ServiceListInt& services, RequestQueueInt& requestQueue,
            ResponseQueueInt& responseQueue, Bus& bus)
            : nps::Task(priority)
            , services(services)
            , requestQueue(requestQueue)
            , responseQueue(responseQueue)
            , bus(bus)
        {
        }

        bool subscribe(Service& service)
        {
            if(services.full() or services.size() >= MAX_SERVICES)
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
                if (service->acceptsRequest(type))
                    return true;
            }

            return false;
        }

        void receiveRequest(const Message& message) override
        {
            enqueue(requestQueue, message);
        }

        void receiveResponse(const Message& message) override
        {
            enqueue(responseQueue, message);
        }

        // No service accepts the type, so the addressed one (if any) can only get the unknown request.
        void handleUnknownRequest(Message& message) override
        {
            if(message.recipient.serviceId < services.size())
                services[message.recipient.serviceId]->handleUnknownRequest(message);
            else
            {
                message.output.status = Message::STATUS_SERVICE_NOT_FOUND;
                bus.respond(message);
            }
        }

        //nps::Task
        virtual void initialize() override
        {
            for (auto& service : services)
                service->initialize();
        }

        virtual bool isReady() const override
        {
            return not requestQueue.empty() or not responseQueue.empty();
        }

        // Responses go first, they finish operations that already run.
        virtual void process()
        {
            if(not responseQueue.empty())
            {
                auto message = responseQueue.front();
                dispatchResponse(*message);
                responseQueue.pop();
                bus.release(message);
            }
            else if(not requestQueue.empty())
            {
                auto message = requestQueue.front();
                dispatchRequest(*message);
                requestQueue.pop();
                bus.release(message);
            }
        }

        //Service::Api
        void send(Service& sender, Message& message) override
        {
            message.sender = sender.getOid();

            if(message.recipient.portId == getId())
                dispatchRequest(message);
            else
                bus.send(message);
        }

        void respond(Message& message) override
        {
            if(message.sender.portId == getId())
                dispatchResponse(message);
            else
                bus.respond(message);
        }

        PortId getPortId() const override
        {
            return getId();
        }

    protected:
        void dispatchRequest(Message& message)
        {
            auto serviceId = message.recipient.serviceId;

            if(serviceId == NULL_SERVICE)
            {
                bool handled = false;

                for (auto& service : services)
                {
                    if (service->acceptsRequest(message.type))
                    {
                        service->handleRequest(message);
                        handled = true;
                    }
                }

                // Without a port in the recipient this is a broadcast, nobody has to answer it.
                if(not handled and message.recipient.portId != NULL_PORT)
                    rejectRequest(message);
            }
            else if(serviceId >= services.size())
                rejectRequest(message);
            else if(services[serviceId]->acceptsRequest(message.type))
                services[serviceId]->handleRequest(message);
            else
                rejectRequest(message, services[serviceId]);
        }

        void dispatchResponse(Message& message)
        {
            if(message.sender.serviceId < services.size())
                services[message.sender.serviceId]->handleResponse(message);
        }

        // The service may change the status in its handler, otherwise the sender gets STATUS_MESSAGE_NOT_SUPPORTED.
        void rejectRequest(Message& message, Service* service = nullptr)
        {

            if(service)
                service->handleUnknownRequest(message);
            else
            {
                message.output.status = Message::STATUS_SERVICE_NOT_FOUND;
                respond(message);
            }
        }

        inline auto& getServices() { return services; }
        inline const auto& getServices() const { return services; }

    private:
        // A message that does not fit is given back to the bus, otherwise its pool slot would leak.
        void enqueue(etl::iqueue<Message*>& queue, const Message& message)
        {
            auto& pending = const_cast<Message&>(message);

            if(queue.full())
                return bus.release(&pending);

            queue.push(&pending);
        }

        ServiceListInt& services;
        RequestQueueInt& requestQueue;
        ResponseQueueInt& responseQueue;
        Bus& bus;
    };


    template <size_t REQUEST_SIZE, size_t RESPONSE_SIZE, size_t SERVICES>
    class ServiceTaskStatic : public ServiceTask
    {
    public:
        ServiceTaskStatic(Priority priority, Bus& bus)
            : ServiceTask(priority, servicesStatic, requestQueueStatic, responseQueueStatic, bus)
        {
        }

    private:
        ServiceList<SERVICES> servicesStatic;
        RequestQueue<REQUEST_SIZE> requestQueueStatic;
        ResponseQueue<RESPONSE_SIZE> responseQueueStatic;
    };
}