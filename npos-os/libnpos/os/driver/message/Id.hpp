#pragma once

/**
 * @file Id.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/os/message/Id.hpp>

namespace npos::os::driver::message
{
    enum Id : ipc::Message::Id
    {
        ERROR = os::message::Id::DRIVER,

        CLOCK_GET_TIME_REQUEST,
        CLOCK_GET_TIME_RESPONSE, 
        CLOCK_SET_TIME_REQUEST,
        CLOCK_SET_TIME_RESPONSE,

        LED_SET_STATE_REQUEST,

        TIMER_REGISTER_REQUEST,
        TIMER_ELAPSED_NOTIFICATION,

        HASH_INTERRUPT,
    };
}