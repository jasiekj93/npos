#pragma once

/**
 * @file Request.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <cstddef>

#include <etl/string.h>

#include <libnpos/kernel/Message.hpp>

namespace npos::kernel::syslog
{
    enum Level : kernel::Message::Input::Syslog::Level
    {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR
    };

    struct Request : public kernel::Message
    {
    public:
        static constexpr auto ID = kernel::OS_SYSLOG_MESSAGE_TYPE;

        Request(Pid sender, Level level, etl::string_view message, bool overflow = false) 
        {
            this->type = ID;
            this->sender = sender;
            this->input.data = reinterpret_cast<const uint8_t*>(message.data());
            this->input.size = message.size();
            this->input.syslog.level = static_cast<kernel::Message::Input::Syslog::Level>(level);
            this->input.syslog.overflow = overflow;
        }

        Level getLevel() const
        {
            return static_cast<Level>(this->input.syslog.level);
        }

        bool hasOverflow() const
        {
            return static_cast<bool>(this->input.syslog.overflow);
        }

        etl::string_view getMessage() const
        {
            return etl::string_view(reinterpret_cast<const char*>(input.data), input.size);
        }
    };
}