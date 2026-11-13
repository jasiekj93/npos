#pragma once

/**
 * @file Close.hpp
 * @author Adrian Szczepanski
 * @date 13-11-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/Tick.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct Close : public ipc::Message
    {
        enum Status : ipc::Message::Status
        {
            SUCCESS = 0x00,
            DEVICE_ERROR = 0x01,
            NOT_INITIALIZED = 0x02,
            LOCKED = 0x03
        };

        Close(ipc::Oid recipient)
        {
            this->type = msg::Type::CLOSE;
            this->recipient = recipient;
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Close& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}