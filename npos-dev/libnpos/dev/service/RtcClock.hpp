#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/optional.h>
#include <etl/string_view.h>
#include <etl/utility.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Clock.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/hal/Rtc.hpp>

namespace npos::dev::service
{
    class RtcClock : public ipc::Service
    {
    public:
        static constexpr Timestamp MAX_TIMESTAMP = 4102444799;
        static constexpr Timestamp MIN_TIMESTAMP = 946684800;

        using DateTime = etl::pair<hal::Rtc::Time, hal::Rtc::Date>;

        static Timestamp fromRtcTime(const hal::Rtc::Time& time, const hal::Rtc::Date& date)
        {
            int32_t year = 2000 + date.year;
            uint32_t month = date.month;
            uint32_t day = date.day;

            year -= (month <= 2);

            const int32_t era =
                (year >= 0 ? year : year - 399) / 400;

            const uint32_t yearOfEra =
                static_cast<uint32_t>(year - era * 400);

            const uint32_t monthPrime =
                month + (month > 2 ? -3 : 9);

            const uint32_t dayOfYear =
                (153 * monthPrime + 2) / 5 + day - 1;

            const uint32_t dayOfEra =
                yearOfEra * 365
                + yearOfEra / 4
                - yearOfEra / 100
                + dayOfYear;

            auto days = era * 146097
                + static_cast<int64_t>(dayOfEra)
                - 719468;

            return days * 86400 + time.hours * 3600 + time.minutes * 60 + time.seconds;
        }

        static etl::optional<DateTime> toRtcTime(Timestamp timestamp)
        {
            if(timestamp < MIN_TIMESTAMP or timestamp > MAX_TIMESTAMP)
                return etl::nullopt;

            int64_t days = timestamp / 86400;
            int64_t secondsOfDay = timestamp % 86400;

            if (secondsOfDay < 0)
            {
                secondsOfDay += 86400;
                --days;
            }

            // Howard Hinnant: civil_from_days

            days += 719468;

            const int64_t era =
                (days >= 0 ? days : days - 146096) / 146097;

            const uint32_t dayOfEra =
                static_cast<uint32_t>(days - era * 146097);

            const uint32_t yearOfEra =
                (dayOfEra
                - dayOfEra / 1460
                + dayOfEra / 36524
                - dayOfEra / 146096)
                / 365;

            int32_t year =
                static_cast<int32_t>(yearOfEra + era * 400);

            const uint32_t dayOfYear =
                dayOfEra
                - (365 * yearOfEra
                + yearOfEra / 4
                - yearOfEra / 100);

            const uint32_t monthPrime =
                (5 * dayOfYear + 2) / 153;

            const uint32_t day =
                dayOfYear
                - (153 * monthPrime + 2) / 5
                + 1;

            const uint32_t month =
                monthPrime + (monthPrime < 10 ? 3 : -9);

            year += (month <= 2);

            hal::Rtc::Time time;
            hal::Rtc::Date date;

            date.year = year - 2000;
            date.month = static_cast<uint8_t>(month);
            date.day = static_cast<uint8_t>(day);
            time.hours = static_cast<uint8_t>(secondsOfDay / 3600);
            time.minutes = static_cast<uint8_t>((secondsOfDay % 3600) / 60);
            time.seconds = static_cast<uint8_t>(secondsOfDay % 60);

            return DateTime{ time, date };
        }

        RtcClock(Api& api, hal::Rtc& rtc, etl::string_view name)
            : ipc::Service(api)
            , rtc(rtc)
            , isInitalized(false)
            , name(name)
        {
        }

        void handle(ipc::Message& message)
        {
            if(message.type == msg::Type::CLOCK)
            {
                auto& request = static_cast<msg::Clock&>(message);

                if(request.getOperation() == msg::Clock::Operation::GET_TIME)
                    return getTime(request);
                else
                    return setTime(request);
            }
            else if(message.type == msg::Type::OPEN)
                return open(message);
        }

        bool accepts(ipc::Message::Type type) const override
        {
            return (type == msg::Type::CLOCK or 
                    type == msg::Type::OPEN);
        }

    protected:
        void getTime(msg::Clock& request)
        {
            if(not isInitalized)
                return api.respond(request.setStatus(msg::Clock::Status::NOT_INITIALIZED));

            hal::Rtc::Time time;
            hal::Rtc::Date date;

            if (not rtc.getTime(time))
                return api.respond(request.setStatus(msg::Clock::Status::DEVICE_FAILURE));

            if (not rtc.getDate(date))
                return api.respond(request.setStatus(msg::Clock::Status::DEVICE_FAILURE));

            return api.respond(request.setStatus(msg::Clock::Status::SUCCESS).setOutputTimestamp(fromRtcTime(time, date)));
        }

        void setTime(msg::Clock& request)
        {
            if(not isInitalized)
                return api.respond(request.setStatus(msg::Clock::Status::NOT_INITIALIZED));

            auto result = toRtcTime(request.getInputTimestamp());

            if(not result.has_value())
                return api.respond(request.setStatus(msg::Clock::Status::INVALID_TIMESTAMP));

            auto [time, date] = result.value();
            
            if(not rtc.setTime(time))
                return api.respond(request.setStatus(msg::Clock::Status::DEVICE_FAILURE));

            if(not rtc.setDate(date))
                return api.respond(request.setStatus(msg::Clock::Status::DEVICE_FAILURE));
            
            return api.respond(request.setStatus(msg::Clock::Status::SUCCESS).setOutputTimestamp(request.getInputTimestamp()));
        }

        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            auto status = msg::Open::Status::SUCCESS;

            if(not isInitalized)
            {
                if(not rtc.initalize())
                    status = msg::Open::Status::INIT_FAILURE;
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