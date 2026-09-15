#pragma once

/**
 * @file MessageId.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/os/message/Id.hpp>

namespace npos::os::driver
{
    enum MessageId : ipc::Message::Id
    {
        CLOCK_GET_TIME_REQUEST = os::message::Id::DRIVER,
        CLOCK_GET_TIME_RESPONSE, 
        CLOCK_SET_TIME_REQUEST,
        CLOCK_SET_TIME_RESPONSE,
    };
}