#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 27-09-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/message/Type.hpp>
#include <libnpos/dev/Timestamp.hpp>

namespace npos::dev::message
{
    struct ClockRequest : public ipc::Message
    {
        enum Operation : ipc::Message::Input::Attributes::Mode
        {
            GET_TIME,
            SET_TIME
        };

        enum Status : ipc::Message::Output::Status
        {
            SUCCESS,
            DEVICE_FAILURE,
            INVALID_TIMESTAMP,
            NOT_INITIALIZED
        };

        ClockRequest(ipc::Oid oid, Operation operation, Timestamp timestamp = 0)
        {
            this->oid = oid;
            this->type = dev::Type::CLOCK;
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