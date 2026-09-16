#pragma once

/**
 * @file Error.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/driver/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::driver::message
{
    struct Error : public os::message::Request
    {
        static constexpr auto ID = Id::ERROR;

        enum Code : uint16_t
        {
            RTC_INITIALIZATION = 0,
        };

        Code code;

        Error(Code c)
            : os::message::Request(ID)
            , code(c)
        {
        }
    };
}