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

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Hash.hpp>
#include <libnpos/dev/msg/Open.hpp>
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
        {
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == msg::Type::HASH)
            {
                auto& request = static_cast<msg::Hash&>(message);

                if(isInitalized == false)
                    return getApi().respond(request.setStatus(msg::Hash::Status::NOT_INITIALIZED));

                if(currentRequest != nullptr)
                    return getApi().respond(request.setStatus(msg::Hash::Status::BUSY));


                if(request.getOperation() == msg::Hash::Operation::ACCUMULATE)
                {
                    request.referenceCount++;
                    currentRequest = &request;
                    hash.accumulateIt(request.getInput());
                }
                else if(request.getOperation() == msg::Hash::Operation::ACCUMULATE_END)
                {
                    if(request.getOutput().size() != hal::Hash::DIGEST_SIZE)
                        return getApi().respond(request.setStatus(msg::Hash::Status::INVALID_OUTPUT_SIZE));

                    request.referenceCount++;
                    currentRequest = &request;
                    hash.accumulateEndIt(request.getInput(), request.getOutput());
                }
            }
            else if (message.type == msg::Type::OPEN)
                open(message);
        }

        bool accepts(ipc::Message::Type type) const override
        {
            return (type == msg::Type::HASH or 
                    type == msg::Type::OPEN);
        }

        void onInterrupt(const hal::Interrupt& interrupt) override
        {
            if(currentRequest == nullptr)
                return;

            if(interrupt.type != hal::Interrupt::Type::HASH)
                return;

            switch (interrupt.code)
            {
            case hal::Hash::Event::INPUT_COMPLETE:
            case hal::Hash::Event::DIGEST_COMPLETE:
            {
                auto* request = currentRequest;
                request->referenceCount--;
                currentRequest = nullptr;
                return getApi().respond(request->setStatus(msg::Hash::Status::SUCCESS));
            }
            
            case hal::Hash::Event::ERROR:
            {
                auto* request = currentRequest;
                request->referenceCount--;
                currentRequest = nullptr;
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

            if(not isInitalized)
            {
                if(not hash.init())
                    status = msg::Open::Status::INIT_FAILURE;
                else
                    isInitalized = true;
            }

            api.respond(request.setStatus(status).setOpened(getOid()));
        }

    private:
        hal::Hash& hash;
        etl::string_view name;
        bool isInitalized = false;
        dev::msg::Hash* currentRequest = nullptr;
    };
}