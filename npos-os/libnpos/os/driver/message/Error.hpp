#pragma once

#include <libnpos/os/driver/message/Id.hpp>

namespace npos::os::driver::message
{
    struct Error
    {
        static constexpr auto ID = Id::ERROR;

        enum Code : uint16_t
        {
            RTC_INITIALIZATION = 0,
        };

        Code code;
    };
}