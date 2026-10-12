#pragma once

/**
 * @file Close.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>

namespace npos::fs::msg::file
{
    struct Close : public ipc::Message
    {
        Close(ipc::Oid file)
        {
            this->recipient = file;
            this->type = ipc::Message::Type::FILE_CLOSE;
        }

        inline ipc::Oid getFile() const
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