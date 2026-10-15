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
        }

        void handleResponse(ipc::Message& message) override {}

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::LED or 
                    type == msg::Type::OPEN);
        }

    protected:
        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            auto status = msg::Open::Status::SUCCESS;
            api.respond(request.setStatus(status).setOpened(getOid()));
        }

    private:
        Leds& leds;
        etl::string_view name;
    };
}