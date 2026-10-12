#pragma once

/**
 * @file Syslog.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc::msg
{
    struct Syslog : public ipc::Message
    {
        enum Level : ipc::Message::Input::Attributes::Mode
        {
            ERROR,
            WARNING,
            INFO,
            DEBUG
        };

        // Text is not copied - it must stay valid until the message is handled.
        Syslog(Level level, etl::string_view text)
        {
            this->type = ipc::OS_SYSLOG_MESSAGE_TYPE;
            this->input.attributes.mode = level;
            this->input.data = reinterpret_cast<const uint8_t*>(text.data());
            this->input.size = text.size();
        }

        inline Level getLevel() const
        {
            return static_cast<Level>(this->input.attributes.mode);
        }

        inline etl::string_view getText() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }
    };
}
