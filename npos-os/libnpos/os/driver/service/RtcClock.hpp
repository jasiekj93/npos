#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/driver/message/Clock.hpp>
#include <libnpos/os/driver/hal/Rtc.hpp>
#include <libnpos/os/Service.hpp>

namespace npos::os::driver::service
{
    class RtcClock : public os::Service
    {
    public:
        explicit RtcClock(hal::Rtc&, os::message::Bus&);

        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) const override;

    private:
        hal::Rtc& rtc;
    };
}