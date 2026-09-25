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
    static constexpr size_t MAX_MESSAGE_SIZE = os::Message::PAYLOAD_SIZE - 1;

    enum Level : uint8_t
    {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR
    };

    struct Request : public os::Message
    {
    public:
        static constexpr auto ID = os::OS_SYSLOG_MESSAGE_TYPE;
        static constexpr Status OVERFLOW_FLAG = 1;
        static constexpr Status NO_OVERFLOW_FLAG = 0;
        static constexpr size_t MESSAGE_OFFSET = 1;
        static constexpr size_t LEVEL_OFFSET = 0;

        Request() 
        {
            this->type = ID;
            this->status = NO_OVERFLOW_FLAG;
            this->data = &this->payload[MESSAGE_OFFSET];
            this->size = 0;
            setLevel(DEBUG);
        }

        void setLevel(Level level)
        {
            this->payload[LEVEL_OFFSET] = static_cast<uint8_t>(level);
        }

        Level getLevel() const
        {
            return static_cast<Level>(this->payload[LEVEL_OFFSET]);
        }

        bool hasOverflow() const
        {
            return this->status == OVERFLOW_FLAG;
        }

        void setOverflow()
        {
            this->status = OVERFLOW_FLAG;
        }

        void clearOverflow()
        {
            this->status = NO_OVERFLOW_FLAG;
        }

        etl::string_view getMessage() const
        {
            return etl::string_view((char*)data, size);
        }
    };
}