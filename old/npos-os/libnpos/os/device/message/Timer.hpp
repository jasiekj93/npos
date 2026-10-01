#pragma once

/**
 * @file Timer.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/Tick.hpp>
#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct TimerRegisterRequest : public os::Message
    {
        static constexpr auto ID = Id::TIMER_REGISTER_REQUEST;

        enum Mode : uint8_t
        {
            ONE_SHOT,
            PERIODIC
        };

        Tick interval;
        Mode mode;

        TimerRegisterRequest() : os::Message(ID) {}
    };

    struct TimerElapsedNotification : public os::Message
    {
        static constexpr auto ID = Id::TIMER_ELAPSED_NOTIFICATION;

        TimerElapsedNotification() : os::Message(ID) {}
    };
}