#include "RtcClock.hpp"

#include <libnpos/device/Error.hpp>
#include <libnpos/device/Clock.hpp>

using namespace npos;
using namespace npos::driver;
using namespace npos::driver::service;

device::Timestamp RtcClock::fromRtcTime(const hal::Rtc::Time& time, const hal::Rtc::Date& date)
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

etl::optional<RtcClock::DateTime> RtcClock::toRtcTime(device::Timestamp timestamp)
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

RtcClock::RtcClock(kernel::Bus& bus, hal::Rtc& rtc)
    : Service(bus)
    , rtc(rtc)
{
}

void RtcClock::initalize()
{
    if(not rtc.initalize())
        broadcast(device::Error(device::Error::Code::RTC_INITIALIZATION));
}

bool RtcClock::accepts(kernel::Message::Type type) const
{
    return type == device::Type::CLOCK; 
}

void RtcClock::onReceive(kernel::Message& message)
{
    auto& request = static_cast<device::ClockRequest&>(message);

    switch(request.getOperation())
    {
    case device::ClockRequest::Operation::GET_TIME:
    {
        hal::Rtc::Time time;
        hal::Rtc::Date date;

        if (not rtc.getTime(time))
            return respond(request.setStatus(device::Status::DEVICE_ERROR));

        if (not rtc.getDate(date))
            return respond(request.setStatus(device::Status::DEVICE_ERROR));

        return respond(request.setStatus(device::Status::OK).setOutputTimestamp(fromRtcTime(time, date)));
    }
    case device::ClockRequest::Operation::SET_TIME:
    {
        auto result = toRtcTime(request.getInputTimestamp());

        if(not result.has_value())
            return respond(request.setStatus(device::Status::INVALID_PARAMETER));

        auto [time, date] = result.value();
        
        if(not rtc.setTime(time))
            return respond(request.setStatus(device::Status::DEVICE_ERROR));

        if(not rtc.setDate(date))
            return respond(request.setStatus(device::Status::DEVICE_ERROR));
        
        return respond(request.setStatus(device::Status::OK).setOutputTimestamp(request.getInputTimestamp()));
    }
    default:
        break;
    }
}