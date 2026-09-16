#include "RtcClock.hpp"

#include <libnpos/os/driver/message/Error.hpp>

using namespace npos;
using namespace npos::os;
using namespace npos::os::driver;
using namespace npos::os::driver::service;

Timestamp RtcClock::fromRtcTime(const hal::Rtc::Time& time, const hal::Rtc::Date& date)
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

etl::optional<RtcClock::DateTime> RtcClock::toRtcTime(Timestamp timestamp)
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

RtcClock::RtcClock(hal::Rtc& rtc, os::message::Bus& bus)
    : Service(bus)
    , rtc(rtc)
{
}

void RtcClock::initalize()
{
    if(not rtc.initalize())
        bus.receive(message::Error(message::Error::Code::RTC_INITIALIZATION));
}

void RtcClock::onReceive(const ipc::Message& message)
{
    switch(message.getId())
    {
    case message::Clock::GetTimeRequest::ID:
    {
        auto& request = static_cast<const message::Clock::GetTimeRequest&>(message);

        hal::Rtc::Time time;
        hal::Rtc::Date date;

        if (not rtc.getTime(time))
            return bus.receive(request.senderId, message::Clock::GetTimeResponse{});

        if (not rtc.getDate(date))
            return bus.receive(request.senderId, message::Clock::GetTimeResponse{});

        return bus.receive(request.senderId, message::Clock::GetTimeResponse{fromRtcTime(time, date)});
    }
    case message::Clock::SetTimeRequest::ID:
    {
        auto& request = static_cast<const message::Clock::SetTimeRequest&>(message);
        auto result = toRtcTime(request.timestamp);

        if(not result.has_value())
            return bus.receive(request.senderId, 
                message::Clock::SetTimeResponse{message::Clock::SetTimeResponse::Status::INVALID_TIMESTAMP});

        auto [time, date] = result.value();

        if(not rtc.setTime(time))
            return bus.receive(request.senderId, 
                message::Clock::SetTimeResponse{message::Clock::SetTimeResponse::Status::RTC_ERROR});

        if(not rtc.setDate(date))
            return bus.receive(request.senderId, 
                message::Clock::SetTimeResponse{message::Clock::SetTimeResponse::Status::RTC_ERROR});
        
        return bus.receive(request.senderId, 
            message::Clock::SetTimeResponse{message::Clock::SetTimeResponse::Status::OK, request.timestamp});
    }
    default:
        break;
    }
}

bool RtcClock::accepts(ipc::Message::Id id) const
{
    return id == message::Clock::GetTimeRequest::ID or
           id == message::Clock::SetTimeRequest::ID;
}
