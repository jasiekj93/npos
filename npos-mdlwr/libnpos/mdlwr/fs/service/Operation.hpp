#pragma once

/**
 * @file Operation.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/ipc/FsmService.hpp>
#include <libnpos/ipc/MessageStore.hpp>
#include <libnpos/fs/Api.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::service
{
    // Base of the services which carry a request out as a series of filesystem requests.
    // The service stays in IDLE until a request comes, then it moves through the states, each one waiting
    // for a response of the filesystem, and answers the request when it is done.
    template<typename Request>
    class Operation : public ipc::FsmService
    {
    public:
        void initialize() override
        {
            start();
        }

    protected:
        using ID = State::ID;

        static constexpr ID IDLE = 0;

        // Every state accepts the request type, but only IDLE takes it. The rest answers BUSY.
        class Step : public State
        {
        public:
            Step(ID id, Operation& operation)
                : State(id, operation)
                , operation(operation)
            {
            }

            bool acceptsRequest(ipc::Message::Type type) const override
            {
                return type == operation.requestType;
            }

            ID handleRequest(ipc::Message& message) override
            {
                operation.getApi().respond(static_cast<Request&>(message).setStatus(msg::Status::BUSY));
                return getId();
            }

        protected:
            Operation& operation;
        };

        class Idle : public Step
        {
        public:
            explicit Idle(Operation& operation)
                : Step(IDLE, operation)
            {
            }

            ID handleRequest(ipc::Message& message) override
            {
                return this->operation.begin(static_cast<Request&>(message));
            }
        };

        // State which waits for the response of type TYPE and passes it to the handler of the service.
        template<typename Service, typename Response, ipc::Message::Type TYPE>
        class Await : public Step
        {
        public:
            using Handler = ID (Service::*)(Response&);

            Await(ID id, Service& service, Handler handler)
                : Step(id, service)
                , service(service)
                , handler(handler)
            {
            }

            ID handleResponse(ipc::Message& message) override
            {
                if(message.type != TYPE)
                    return this->getId();

                return (service.*handler)(static_cast<Response&>(message));
            }

        private:
            Service& service;
            Handler handler;
        };

        Operation(Api& api, State* states[], ipc::Message::Type requestType)
            : ipc::FsmService(api, states)
            , requestType(requestType)
            , current(*this)
        {
        }

        // Called in IDLE with the new request. Returns the state to continue in, IDLE when the request is already answered.
        virtual ID begin(Request& request) = 0;

        ID refuse(Request& request, msg::Status status)
        {
            getApi().respond(request.setStatus(status));
            return IDLE;
        }

        // Answers the stored request.
        ID finish(msg::Status status)
        {
            if(auto* request = current.release())
                getApi().respond(request->setStatus(status));

            return IDLE;
        }

        ipc::Message::Type requestType;
        ipc::MessageStore<Request> current;
    };
}
