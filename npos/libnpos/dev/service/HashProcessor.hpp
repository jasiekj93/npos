#pragma once

/**
 * @file HashProcessor.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <etl/optional.h>
#include <etl/string_view.h>
#include <etl/utility.h>
#include <etl/vector.h>
#include <etl/queue.h>

#include <libnpos/ipc/MessageStore.hpp>
#include <libnpos/dev/InterruptService.hpp>
#include <libnpos/dev/msg/Hash.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Close.hpp>
#include <libnpos/dev/hal/Hash.hpp>

namespace npos::dev::service
{
    class HashProcessor : public InterruptService
    {
    public:
        HashProcessor(Api& api, InterruptQueueInt& interruptQueue, hal::Hash& hash, etl::string_view name)
            : InterruptService(api, interruptQueue, hash)
            , hash(hash)
            , name(name)
            , currentRequest(*this)
            , owner(ipc::NULL_OID)
        {
        }

        void handleRequest(ipc::Message& message) override
        {
            if(message.type == msg::Type::HASH)
            {
                auto& request = static_cast<msg::Hash&>(message);

                if(owner == ipc::NULL_OID)
                    return getApi().respond(request.setStatus(msg::Hash::Status::NOT_INITIALIZED));

                if(owner != message.sender)
                    return getApi().respond(request.setStatus(msg::Hash::Status::LOCKED));

                if(currentRequest.hasValue())
                    return getApi().respond(request.setStatus(msg::Hash::Status::BUSY));

                if(request.getOperation() == msg::Hash::Operation::ACCUMULATE)
                {
                    currentRequest.store(request); 

                    if(not hash.accumulateIt(request.getInput()))
                    {
                        currentRequest.release();
                        return getApi().respond(request.setStatus(msg::Hash::Status::DEVICE_FAILURE));
                    }
                }
                else if(request.getOperation() == msg::Hash::Operation::ACCUMULATE_END)
                {
                    if(request.getOutput().size() != hal::Hash::DIGEST_SIZE)
                        return getApi().respond(request.setStatus(msg::Hash::Status::INVALID_OUTPUT_SIZE));

                    currentRequest.store(request);

                    if(not hash.accumulateEndIt(request.getInput(), request.getOutput()))
                    {
                        currentRequest.release();
                        return getApi().respond(request.setStatus(msg::Hash::Status::DEVICE_FAILURE));
                    }
                }
                else
                    return handleUnknownRequest(message);
            }
            else if (message.type == msg::Type::OPEN)
                open(message);
            else if (message.type == msg::Type::CLOSE)
                close(message);
        }

        void handleResponse(ipc::Message& message) override {}

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::HASH or 
                    type == msg::Type::OPEN or
                    type == msg::Type::CLOSE);
        }

        void onInterrupt(const hal::Interrupt& interrupt) override
        {
            if(not currentRequest.hasValue())
                return;

            if(interrupt.type != hal::Interrupt::Type::HASH_INT)
                return;

            switch (interrupt.code)
            {
            case hal::Hash::Event::INPUT_COMPLETE:
            case hal::Hash::Event::DIGEST_COMPLETE:
            {
                auto* request = currentRequest.release();
                return getApi().respond(request->setStatus(msg::Hash::Status::SUCCESS));
            }
            
            case hal::Hash::Event::ERROR:
            {
                auto* request = currentRequest.release();
                return getApi().respond(request->setStatus(msg::Hash::Status::DEVICE_FAILURE));
            }
            default:
                break;
            }
        }

    protected:
        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            auto status = msg::Open::Status::SUCCESS;

            if(owner == request.sender)
                return api.respond(request.setStatus(msg::Open::Status::ALREADY_OPENED));

            if(owner != ipc::NULL_OID)
                return api.respond(request.setStatus(msg::Open::Status::LOCKED));

            if(not hash.init())
                status = msg::Open::Status::INIT_FAILURE;
            else
                owner = request.sender;

            api.respond(request.setStatus(status).setOpened(getOid()));
        }

        void close(ipc::Message& message)
        {
            auto& request = static_cast<msg::Close&>(message);

            if(owner == ipc::NULL_OID)
                return api.respond(request.setStatus(msg::Close::Status::NOT_INITIALIZED));

            if(owner != message.sender)
                return api.respond(request.setStatus(msg::Close::Status::LOCKED));

            owner = ipc::NULL_OID;
            hash.deinit();
            api.respond(request.setStatus(msg::Close::Status::SUCCESS));
        }

    private:
        hal::Hash& hash;
        etl::string_view name;
        ipc::Oid owner = ipc::NULL_OID;
        ipc::MessageStore<dev::msg::Hash> currentRequest;
    };
}