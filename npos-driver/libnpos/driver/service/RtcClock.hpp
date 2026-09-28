#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */


#include <libnpos/driver/hal/Rtc.hpp>
#include <libnpos/kernel/Service.hpp>
#include <libnpos/device/Timestamp.hpp>

namespace npos::driver::service
{
    class RtcClock : public kernel::Service
    {
    public:
        static constexpr device::Timestamp MAX_TIMESTAMP = 4102444799;
        static constexpr device::Timestamp MIN_TIMESTAMP = 946684800;

        using DateTime = etl::pair<hal::Rtc::Time, hal::Rtc::Date>;

        static device::Timestamp fromRtcTime(const hal::Rtc::Time&, const hal::Rtc::Date&);
        static etl::optional<DateTime> toRtcTime(device::Timestamp);

        explicit RtcClock(kernel::Bus&, hal::Rtc&);

        void initalize() override;
        bool accepts(kernel::Message::Type) const override;

    protected:
        void onReceive(kernel::Message&) override;

    private:
        hal::Rtc& rtc;
    };

    // class RtcClockProcess : public os::QueuedProcess<message::Clock::GetTimeRequest, message::Clock::SetTimeRequest>
    // {
    // public:
    //     RtcClockProcess(Priority p, MessageQueue& mq, os::message::Bus& bus, hal::Rtc& rtc)
    //         : os::QueuedProcess<message::Clock::GetTimeRequest, message::Clock::SetTimeRequest>(p, mq, service)
    //         , service(rtc, bus)
    //     {
    //     }

    // private:
    //     RtcClock service;
    // };
}