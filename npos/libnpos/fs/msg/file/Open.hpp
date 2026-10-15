#pragma once

/**
 * @file Open.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/string_view.h>
#include <etl/initializer_list.h>

#include <libnpos/fs/Flags.hpp>
#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::file
{
    struct Open : public ipc::Message
    {
        Open(etl::string_view path, ipc::Message::Input::Open::Flags flags)
        {
            this->type = msg::Type::FILE_OPEN;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
            this->input.open.flags = flags;
        }

        inline etl::string_view getPath() const
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

        inline size_t getSize() const
        {
            return this->output.open.size;
        }

        inline Open& setSize(size_t size)
        {
            this->output.open.size = size;
            return *this;
        }
    };
}