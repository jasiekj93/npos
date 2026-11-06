#pragma once

/**
 * @file Read.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdint>
#include <cstring>

#include <etl/string_view.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/fs/Info.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::dir
{
    // Reads the next entry of an opened directory, END_OF_DIRECTORY status means there are no more.
    struct Read : public ipc::Message
    {
        Read(ipc::Oid dir, Info& entry)
        {
            this->recipient = dir;
            this->type = msg::Type::DIR_READ;
            this->output.data = reinterpret_cast<uint8_t*>(&entry);
            this->output.size = sizeof(Info);
        }

        inline ipc::Oid getDir() const
        {
            return this->recipient;
        }

        inline Info& getEntry() const
        {
            return *reinterpret_cast<Info*>(this->output.data);
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Read& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
