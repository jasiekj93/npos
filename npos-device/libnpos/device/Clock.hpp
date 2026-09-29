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
        enum Operation : kernel::Message::Input::Attributes::Type
        {
            GET_TIME,
            SET_TIME
        };

        ClockRequest(Operation operation, Timestamp timestamp = 0)
        {
            this->type = device::Type::CLOCK;
            this->input.attributes.type = operation;
            this->input.attributes.value = timestamp;
        }

        inline Timestamp getInputTimestamp() const
        {
            return this->input.attributes.value;
        }

        inline Timestamp getOutputTimestamp() const
        {
            return this->output.attributes.value;
        }

        inline ClockRequest& setOutputTimestamp(Timestamp timestamp)
        {
            this->output.attributes.value = timestamp;
            return *this;
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.attributes.type);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline ClockRequest& setStatus(Status status)
        {
            this->output.status = static_cast<kernel::Message::Status>(status);
            return *this;
        }
    };
}