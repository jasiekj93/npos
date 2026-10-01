#pragma once

/**
 * @file Clock.hpp
 * @author Adrian Szczepanski
 * @date 27-09-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/msg/Type.hpp>
#include <libnpos/dev/Timestamp.hpp>

namespace npos::dev::msg
{
    struct Clock: public ipc::Message
    {
        enum Operation : ipc::Message::Input::Attributes::Mode
        {
            GET_TIME,
            SET_TIME
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS,
            DEVICE_FAILURE,
            INVALID_TIMESTAMP,
            NOT_INITIALIZED
        };

        Clock(ipc::Oid oid, Operation operation, Timestamp timestamp = 0)
        {
            this->recipient = oid;
            this->type = msg::Type::CLOCK;
            this->input.attributes.mode = operation;
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

        inline Clock& setOutputTimestamp(Timestamp timestamp)
        {
            this->output.attributes.value = timestamp;
            return *this;
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.attributes.mode);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Clock& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}