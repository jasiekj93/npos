#pragma once

/**
 * @file Request.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <cstddef>

#include <etl/string.h>

#include <libnpos/os/Message.hpp>

namespace npos::os::syslog
{
    static constexpr size_t MAX_MESSAGE_SIZE = 128;

    using Payload = etl::string<MAX_MESSAGE_SIZE>;

    enum Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    struct Request : public os::Message
    {
    public:
        static constexpr auto ID = os::Message::OS_SYSLOG;

        Level level;
        Payload message;
        bool overflowFlag;

        Request() : os::Message(ID) {}
    };
}