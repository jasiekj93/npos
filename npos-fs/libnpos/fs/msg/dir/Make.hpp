#pragma once

/**
 * @file Make.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::dir
{
    struct Make : public ipc::Message
    {
        // The path must be null terminated.
        explicit Make(etl::string_view path)
        {
            this->type = msg::Type::DIR_MAKE;
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

        inline Make& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
