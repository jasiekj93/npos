#pragma once

/**
 * @file LedHandler.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <etl/optional.h>
#include <etl/string_view.h>
#include <etl/utility.h>
#include <etl/vector.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Led.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Close.hpp>
#include <libnpos/dev/hal/Led.hpp>

namespace npos::dev::service
{
    class LedHandler : public ipc::Service
    {
    public:
        using Leds = etl::ivector<hal::Led*>;

        LedHandler(Api& api, Leds& leds, etl::string_view name)
            : ipc::Service(api)
            , leds(leds)
            , name(name)
            , owner(ipc::NULL_OID)
        {
        }

        void handleRequest(ipc::Message& message) override
        {
            if(message.type == msg::Type::LED)
            {
                auto& request = static_cast<msg::Led&>(message);

                if(request.getId() >= leds.size())
                    return getApi().respond(request.setStatus(msg::Led::Status::LED_NOT_FOUND));

                if(request.getOperation() == msg::Led::Operation::ON)
                    leds[request.getId()]->on();
                else if(request.getOperation() == msg::Led::Operation::OFF)
                    leds[request.getId()]->off();
                else if(request.getOperation() == msg::Led::Operation::TOGGLE)
                    leds[request.getId()]->toggle();
                else
                    return handleUnknownRequest(message);

                return getApi().respond(request.setStatus(msg::Led::Status::SUCCESS));
            }
            else if(message.type == msg::Type::OPEN)
                return open(message);
            else if(message.type == msg::Type::CLOSE)
                return close(message);
        }

        void handleResponse(ipc::Message& message) override {}

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::LED or 
                    type == msg::Type::OPEN or
                    type == msg::Type::CLOSE);
        }

    protected:
        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            if(owner == request.sender)
                return api.respond(request.setStatus(msg::Open::Status::ALREADY_OPENED));

            if(owner != ipc::NULL_OID)
                return api.respond(request.setStatus(msg::Open::Status::LOCKED));

            owner = request.sender;
            auto status = msg::Open::Status::SUCCESS;
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
            api.respond(request.setStatus(msg::Close::Status::SUCCESS));
        }

    private:
        Leds& leds;
        etl::string_view name;
        ipc::Oid owner = ipc::NULL_OID;
    };
}