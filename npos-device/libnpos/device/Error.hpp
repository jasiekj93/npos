#pragma once

/**
 * @file Error.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <libnpos/device/Type.hpp>

namespace npos::device
{
    struct Error : public kernel::Message
    {
        enum Code : kernel::Message::Attributes::Value
        {
            UNKNOWN,
            RTC_INITIALIZATION,
        };

        explicit Error(Code code)
        {
            this->type = device::Type::ERROR;
            this->attributes.value = static_cast<uint32_t>(code);
        }
    };
}