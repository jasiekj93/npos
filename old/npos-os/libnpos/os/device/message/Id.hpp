#pragma once

/**
 * @file Id.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/os/Message.hpp>

namespace npos::os::device::message
{
    enum Id : os::Message::Type
    {
        ERROR = os::DEVICE_MESSAGE_TYPE,

        CLOCK_GET_TIME,
        CLOCK_SET_TIME,

        LED_SET_STATE,

        TIMER_REGISTER_REQUEST,
        TIMER_ELAPSED_NOTIFICATION,

        HASH_COMPUTE,

        SYSTEM_RESET_REQUEST,

        BACKUP_REGISTER,
    };
}