#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 27-09-2026
 */

#include <libnpos/device/Type.hpp>
#include <libnpos/device/Timestamp.hpp>
#include <libnpos/device/Status.hpp>

namespace npos::device
{
    struct ClockRequest : public kernel::Message
    {
        enum Operation : kernel::Message::Attributes::Type
        {
            GET_TIME,
            SET_TIME
        };

        ClockRequest(Operation operation, Timestamp timestamp = 0)
        {
            this->type = device::Type::CLOCK;
            this->attributes.type = operation;
            this->attributes.value = timestamp;
        }

        inline Timestamp getTimestamp() const
        {
            return this->attributes.value;
        }

        inline ClockRequest& setTimestamp(Timestamp timestamp)
        {
            this->attributes.value = timestamp;
            return *this;
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->attributes.type);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->status);
        }

        inline ClockRequest& setStatus(Status status)
        {
            this->status = static_cast<kernel::Message::Status>(status);
            return *this;
        }
    };
}