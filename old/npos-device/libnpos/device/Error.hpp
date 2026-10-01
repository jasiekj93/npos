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
        enum Code : kernel::Message::Input::Attributes::Value
        {
            UNKNOWN,
            RTC_INITIALIZATION,
            UART_INITIALIZATION,
        };

        explicit Error(Code code, kernel::Message::ObjectId objectId = 0)
        {
            this->type = device::Type::ERROR;
            this->input.attributes.value = static_cast<uint32_t>(code);
            this->object = objectId;
        }
    };
}