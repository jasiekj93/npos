#pragma once

/**
 * @file Timer.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/Tick.hpp>
#include <libnpos/os/driver/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::driver::message
{
    struct TimerRegisterRequest : public os::message::Request
    {
        static constexpr auto ID = Id::TIMER_REGISTER_REQUEST;

        Tick interval;

        TimerRegisterRequest(Request::SenderId senderId, Tick interval) 
            : os::message::Request(ID)
            , senderId(senderId)
            , interval(interval) {}
    };

    struct TimerElapsedNotification : public os::message::Response
    {
        static constexpr auto ID = Id::TIMER_ELAPSED_NOTIFICATION;

        TimerElapsedNotification() 
            : os::message::Response(ID) {}
    };
}