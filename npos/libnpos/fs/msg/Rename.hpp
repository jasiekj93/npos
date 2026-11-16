#pragma once

/**
 * @file Rename.hpp
 * @author Adrian Szczepanski
 * @date 16-11-2026
 */

#include <cstdint>

#include <etl/string_view.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg
{
    struct Rename : public ipc::Message
    {
        Rename(etl::string_view oldPath, etl::string_view newPath)
        {
            this->type = msg::Type::RENAME;
            this->input.data = reinterpret_cast<const uint8_t*>(oldPath.data());
            this->input.size = oldPath.size();
            this->output.data = reinterpret_cast<uint8_t*>(const_cast<char*>(newPath.data()));
            this->output.size = newPath.size();
        }

        inline etl::string_view getOldPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline etl::string_view getNewPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->output.data), this->output.size);
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Rename& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
