#pragma once

/**
 * @file SysLog.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <cstddef>

#include <etl/string.h>

#include <libnpos/os/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::message
{
    struct SysLogRequest : public Request
    {
    public:
        static constexpr auto ID = os::message::Id::SYSLOG;
        static constexpr size_t MAX_MESSAGE_SIZE = 128;

        using Message = etl::string<MAX_MESSAGE_SIZE>;

        enum Level
        {
            DEBUG,
            INFO,
            WARNING,
            ERROR
        };

        Level level;
        Message message;
        bool overflowFlag;

        SysLogRequest(Request::SenderId senderId)
            : Request(ID, senderId) {}
    };
}