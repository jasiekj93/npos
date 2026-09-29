#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/optional.hpp>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/message/Clock.hpp>
#include <libnpos/dev/message/Open.hpp>
#include <libnpos/dev/hal/Rtc.hpp>

namespace npos::dev::service
{
    class RtcClock : public ipc::Service
    {
    public:
        static constexpr device::Timestamp MAX_TIMESTAMP = 4102444799;
        static constexpr device::Timestamp MIN_TIMESTAMP = 946684800;

        using DateTime = etl::pair<hal::Rtc::Time, hal::Rtc::Date>;

        static Timestamp fromRtcTime(const hal::Rtc::Time&, const hal::Rtc::Date&)
        {

        }

        static etl::optional<DateTime> toRtcTime(Timestamp)
        {

        }

        RtcClock(Api& api, hal::Rtc& rtc, etl::string_view name)
            : ipc::Service(api)
            , rtc(rtc)
            , isInitalized(false)
            , name(name)
        {
        }

        void handle(Message& message)
        {
            if(message.type == message::Type::CLOCK)
            {
                auto& request = static_cast<message::ClockRequest&>(message);

                if(request.getOperation() == message::Clock::Operation::GET_TIME)
                    getTime(request);
                else
                    setTime(request);
            }
            else if(message.type == message::Type::OPEN)
                open(message);
        }

        bool accepts(Message::Type) const override
        {
            return (type == message::Type::CLOCK or 
                    type == message::Type::OPEN);
        }

    protected:
        void getTime(message::ClockRequest& request)
        {
            if(not isInitalized)
                return api.respond(request.setStatus(message::ClockRequest::Status::NOT_INITIALIZED));

            //todo
        }

        void setTime(message::ClockRequest& request)
        {
            if(not isInitalized)
                return api.respond(request.setStatus(message::ClockRequest::Status::NOT_INITIALIZED));

            //todo
        }

        void open(Message& message)
        {
            auto& request = static_cast<message::OpenRequest&>(message);

            if(request.getName() != name)
                return;

            auto status = message::OpenRequest::Status::SUCCESS;

            if(not isInitalized)
            {
                if(not rtc.initalize())
                    status = message::OpenRequest::Status::INIT_FAILURE;
                else
                    isInitalized = true;
            }

            ipc::Oid oid { api.getPortId(), getId() };
            api.respond(request.setStatus(status).setOpened(oid));
        }

    private:
        hal::Rtc& rtc;
        bool isInitalized;
        etl::string_view name;
    };
}