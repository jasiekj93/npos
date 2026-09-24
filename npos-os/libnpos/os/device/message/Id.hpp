#pragma once

/**
 * @file Id.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/os/Message.hpp>

namespace npos::os::device::message
{
    enum Id : os::Message::Id
    {
        ERROR = os::Message::DEVICE,

        CLOCK_GET_TIME_REQUEST,
        CLOCK_GET_TIME_RESPONSE, 
        CLOCK_SET_TIME_REQUEST,
        CLOCK_SET_TIME_RESPONSE,

        LED_SET_STATE_REQUEST,

        TIMER_REGISTER_REQUEST,
        TIMER_ELAPSED_NOTIFICATION,

        HASH_COMPUTE_REQUEST,
        HASH_COMPUTE_RESPONSE,

        SYSTEM_RESET_REQUEST,

        BACKUP_REGISTER_REQUEST,
        BACKUP_REGISTER_RESPONSE,
    };
}