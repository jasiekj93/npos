#pragma once

/**
 * @file Open.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/string_view.h>
#include <etl/initializer_list.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::file
{
    struct Open : public ipc::Message
    {
        enum Flag : ipc::Message::Input::Open::Flags
        {
            READ_ONLY = 0x01,
            WRITE_ONLY = 0x02,
            READ_WRITE = 0x03,
            CREATE = 0x0100,
            EXCLUSIVE = 0x0200,
            TRUNCATE = 0x0400,
            APPEND = 0x0800,
        };

        using Flags = etl::initializer_list<Flag>;

        Open(etl::string_view name, Flags flags)
        {
            this->type = msg::Type::FILE_OPEN;
            this->input.data = reinterpret_cast<const uint8_t*>(name.data());
            this->input.size = name.size();
            this->input.open.flags = 0;

            for (auto flag : flags)
            {
                this->input.open.flags |= static_cast<uint32_t>(flag);
            }
        }

        inline etl::string_view getName() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline ipc::Message::Input::Open::Flags getFlags() const
        {
            return this->input.open.flags;
        }

        inline ipc::Oid getOpened() const
        {
            return this->output.open.opened;
        }

        inline Open& setOpened(ipc::Oid opened)
        {
            this->output.open.opened = opened;
            return *this;
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Open& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}