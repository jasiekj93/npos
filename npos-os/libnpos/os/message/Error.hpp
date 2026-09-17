#pragma once

/**
 * @file Error.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::message
{
    struct Error : public ipc::Message
    {
        static constexpr auto ID = message::Id::SYSTEM_ERROR;

        enum Code : uint8_t
        {
            MESSAGE_QUEUE_FULL = 0,
        };

        Code code;

        Error(Code c)
            : ipc::Message(ID)
            , code(c)
        {
        }
    };
}