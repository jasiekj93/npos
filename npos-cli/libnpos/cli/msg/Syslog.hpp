#pragma once

/**
 * @file Syslog.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/cli/Log.hpp>
#include <libnpos/cli/msg/Type.hpp>

namespace npos::cli::msg
{
    struct Syslog : public ipc::Message
    {
        Syslog(ipc::Oid sender, log::Level level, etl::string_view message, bool overflow)
        {
            this->type = msg::Type::SYSLOG;
            this->input.link.target = sender;
            this->input.link.attributes.value = level;
            this->input.link.attributes.mode = overflow;
            this->input.data = reinterpret_cast<const uint8_t*>(message.data());
            this->input.size = message.size();
        }

        inline log::Level getLevel() const
        {
            return static_cast<log::Level>(this->input.link.attributes.value);
        }

        inline bool getOverflow() const
        {
            return this->input.link.attributes.mode;
        }

        inline etl::string_view getMessage() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline ipc::Oid getSender() const
        {
            return this->input.link.target;
        }
    };
}