#pragma once

/**
 * @file Error.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/device/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::device::message
{
    struct Error : public ipc::Message
    {
        static constexpr auto ID = device::message::Id::ERROR;

        enum Code : uint16_t
        {
            RTC_INITIALIZATION = 0,
        };

        Code code;

        Error(Code c)
            : ipc::Message(ID)
            , code(c)
        {
        }
    };
}