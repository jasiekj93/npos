#pragma once

/**
 * @file Close.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::dir
{
    struct Close : public ipc::Message
    {
        explicit Close(ipc::Oid dir)
        {
            this->recipient = dir;
            this->type = msg::Type::DIR_CLOSE;
        }

        inline ipc::Oid getDir() const
        {
            return this->recipient;
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Close& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
