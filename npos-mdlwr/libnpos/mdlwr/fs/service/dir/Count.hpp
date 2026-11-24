#pragma once

/**
 * @file Count.hpp
 * @author Adrian Szczepanski
 * @date 24-11-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/ipc/MessageStore.hpp>
#include <libnpos/fs/Api.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Count.hpp>

namespace npos::mdlwr::fs::service::dir
{
    class Count : public ipc::Service
    {
    public:
        Count(Api& api) 
            : ipc::Service(api)
            , currentRequest(*this)
        {

        }

        void handleRequest(ipc::Message& message) override
        {
            auto& request = static_cast<msg::dir::Count&>(message);

            if(currentRequest.hasValue())
                return api.respond(request.setStatus(msg::Status::BUSY));

            currentRequest.store(request);
            count = 0;
            operationError = false;
            npos::fs::api::dir::open(*this, request.getPath());
        }

        void handleResponse(ipc::Message& message) override
        {
            if(not currentRequest.hasValue()) 
                return;

            switch(message.type)
            {
                case npos::fs::msg::Type::DIR_OPEN:
                    onDirOpen(static_cast<npos::fs::msg::dir::Open&>(message));
                    break;
                case npos::fs::msg::Type::DIR_READ:
                    onDirRead(static_cast<npos::fs::msg::dir::Read&>(message));
                    break;
                case npos::fs::msg::Type::DIR_CLOSE:
                    onDirClose(static_cast<npos::fs::msg::dir::Close&>(message));
                    break;
                default:
                    break;
            }
        }

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return type == msg::Type::DIR_COUNT;
        }

    protected:
        void onDirOpen(npos::fs::msg::dir::Open& response)
        {
            if(response.getStatus() == npos::fs::msg::Status::LOCKED)
                return api.respond(currentRequest.release()->setStatus(msg::Status::BUSY));
            else if(response.getStatus() == npos::fs::msg::Status::NOT_A_DIRECTORY)
                return api.respond(currentRequest.release()->setStatus(msg::Status::NOT_A_DIRECTORY));
            else if(response.getStatus() == npos::fs::msg::Status::NO_DIR_ENTRY)
                return api.respond(currentRequest.release()->setStatus(msg::Status::DIR_NOT_FOUND));
            else if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                directory = response.getOpened();
                npos::fs::api::dir::read(*this, directory, info);
            }
            else
                api.respond(currentRequest.release()->setStatus(msg::Status::DEVICE_ERROR));
        }

        void onDirRead(npos::fs::msg::dir::Read& response)
        {
            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
            {
                ++count;
                npos::fs::api::dir::read(*this, directory, info);
            }
            else if(response.getStatus() == npos::fs::msg::Status::END_OF_DIRECTORY)
                npos::fs::api::dir::close(*this, directory);
            else
            {
                operationError = true;
                npos::fs::api::dir::close(*this, directory);
            }

        }

        void onDirClose(npos::fs::msg::dir::Close& response)
        {
            auto* request = currentRequest.release();

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                operationError = true;

            if(operationError)
            {
                request->setStatus(msg::Status::DEVICE_ERROR);
                request->setCount(0);
            }
            else
            {
                request->setStatus(msg::Status::SUCCESS);
                request->setCount(count);
            }

            api.respond(*request);
        }

    private:
        ipc::MessageStore<msg::dir::Count> currentRequest;
        ipc::Oid directory;
        npos::fs::Info info;
        unsigned count;
        bool operationError;
    };
}