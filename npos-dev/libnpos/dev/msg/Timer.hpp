#pragma once

/**
 * @file Timer.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/Tick.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct Timer : public ipc::Message
    {
        enum Mode : ipc::Message::Input::Attributes::Mode
        {
            SINGLE_SHOT,
            REPEATING,
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS,
            NO_MEMORY
        };

        Timer(ipc::Oid device, ipc::Oid sender, Mode mode, Tick interval)
        {
            this->recipient = device;
            this->type = msg::Type::TIMER;
            this->input.link.target = device;
            this->input.link.attributes.mode = static_cast<ipc::Message::Input::Attributes::Mode>(mode);
            this->input.link.attributes.value = static_cast<ipc::Message::Input::Attributes::Value>(interval);
        }

        inline Mode getMode() const
        {
            return static_cast<Mode>(this->input.link.attributes.mode);
        }

        inline Tick getInterval() const
        {
            return static_cast<Tick>(this->input.link.attributes.value);
        }

        inline ipc::Oid getTarget() const
        {
            return this->input.link.target;
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Timer& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}