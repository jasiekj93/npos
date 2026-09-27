#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 27-09-2026
 */

#include <libnpos/device/Type.hpp>
#include <libnpos/device/Timestamp.hpp>

namespace npos::device
{
    struct Clock : public kernel::Message
    {
        enum Operation : kernel::Message::Attributes::Type
        {
            GET_TIME,
            SET_TIME
        };

        Clock(Operation operation, Timestamp timestamp = 0)
        {
            this->type = Type::CLOCK;
            this->attributes.type = operation;
            this->attributes.value = timestamp;
        }

        inline Timestamp getTimestamp() const
        {
            return this->attributes.value;
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->attributes.type);
        }
    };
}