#pragma once

/**
 * @file Request.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/Message.hpp>

namespace npos::os::error
{
    enum Code : uint8_t
    {
        MESSAGE_QUEUE_FULL = 0,
        MESSAGE_POOL_FULL = 1,
    };

    struct Request : public os::Message
    {
        static constexpr auto ID = os::OS_ERROR_MESSAGE_TYPE;

        Code code;

        Request(Code c)
            : os::Message(ID)
            , code(c)
        {
        }
    };
}