#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */


#include <libnpos/driver/hal/Rtc.hpp>
#include <libnpos/kernel/Service.hpp>

namespace npos::driver::service
{
    class RtcClock : public kernel::Service
    {
    public:
        static constexpr os::Timestamp MAX_TIMESTAMP = 4102444799;
        static constexpr os::Timestamp MIN_TIMESTAMP = 946684800;

        using DateTime = etl::pair<hal::Rtc::Time, hal::Rtc::Date>;

        static os::Timestamp fromRtcTime(const hal::Rtc::Time&, const hal::Rtc::Date&);
        static etl::optional<DateTime> toRtcTime(os::Timestamp);

        explicit RtcClock(kernel::Bus&, hal::Rtc&);

        void initalize() override;
        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) const override;

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