#pragma once

/**
 * @file Heartbeat.hpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/Api.hpp>

namespace npos::dev::service
{
    // Opens the led device and toggles one of its leds every interval ticks.
    class Heartbeat : public ipc::Service
    {
    public:
        Heartbeat(Api& api, etl::string_view ledName, msg::Led::Id ledId, Tick interval)
            : ipc::Service(api)
            , ledName(ledName)
            , ledId(ledId)
            , interval(interval)
        {
        }

        void initialize() override
        {
            api::open(*this, ledName);
        }

        void handleRequest(ipc::Message&) override {}

        void handleResponse(ipc::Message& message) override
        {
            if(message.type == msg::Type::OPEN)
                onOpen(static_cast<msg::Open&>(message));
            else if(message.type == msg::Type::TIMER)
                onTimer(static_cast<msg::Timer&>(message));
        }

        bool acceptsRequest(ipc::Message::Type) const override { return false; }

    private:
        void onOpen(msg::Open& response)
        {
            if(started or response.getStatus() != msg::Open::Status::SUCCESS)
                return;

            led = response.getOpened();
            started = true;
            api::timerRegister(*this, msg::Timer::Mode::REPEATING, interval);
        }

        void onTimer(msg::Timer& response)
        {
            if(started and response.getStatus() == msg::Timer::Status::SUCCESS)
                api::ledToggle(*this, led, ledId);
        }

        etl::string_view ledName;
        msg::Led::Id ledId;
        Tick interval;
        ipc::Oid led = ipc::NULL_OID;
        bool started = false;
    };
}
