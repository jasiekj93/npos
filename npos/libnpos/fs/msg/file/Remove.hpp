#pragma once

/**
 * @file Remove.hpp
 * @author Adrian Szczepanski
 * @date 30-10-2026
 */

#include <etl/string_view.h>
#include <etl/initializer_list.h>

#include <libnpos/fs/Flags.hpp>
#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::file
{
    struct Remove : public ipc::Message
    {
        Remove(etl::string_view path)
        {
            this->type = msg::Type::FILE_REMOVE;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
        }

        inline etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Remove& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}