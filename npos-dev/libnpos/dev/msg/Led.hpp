#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct Led: public ipc::Message
    {
        using Id = ipc::Message::Input::Attributes::Value;

        enum Operation : ipc::Message::Input::Attributes::Mode
        {
            OFF,
            ON,
            TOGGLE
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS,
            LED_NOT_FOUND
        };

        Led(ipc::Oid oid, Id ledId, Operation operation)
        {
            this->recipient = oid;
            this->type = msg::Type::LED; 
            this->input.attributes.mode = operation;
            this->input.attributes.value = ledId;
        }

        inline Id getId() const
        {
            return static_cast<Id>(this->input.attributes.value);
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.attributes.mode);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Led& setStatus(Status status)
        {
            this->output.status = static_cast<Status>(status);
            return *this;
        }
    };
}