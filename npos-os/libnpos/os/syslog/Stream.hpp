#pragma once

/**
 * @file Stream.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/span.h>
#include <etl/string_stream.h>

#include <libnpos/os/message/Bus.hpp>
#include <libnpos/os/message/SysLog.hpp>

namespace npos::os::syslog
{
    struct Endl {};
    static Endl endl;

    class Stream
    {
    public:
        static constexpr size_t MAX_NUMBER_LENGTH = 20; // max length of long in decimal

        Stream(message::Request::SenderId senderId, message::SysLogRequest::Level level, message::Bus& bus);

        Stream& operator<<(etl::string_view);
        Stream& operator<<(const char*);
        Stream& operator<<(const etl::ivector<uint8_t>&);
        Stream& operator<<(const etl::span<const uint8_t>&);
        Stream& operator<<(bool); 
        Stream& operator<<(int); 
        Stream& operator<<(uint8_t);
        Stream& operator<<(unsigned int);
        Stream& operator<<(unsigned long);

        Stream& operator<<(const etl::format_spec&);
        Stream& operator<<(const Endl&);

    private:
        message::SysLogRequest request;
        message::Bus& bus;
        etl::string_stream stream;
        etl::string<MAX_NUMBER_LENGTH> numberBuffer;
    };

    Stream log(message::Request::SenderId senderId, message::SysLogRequest::Level level, message::Bus& bus);
    void setLevel(message::SysLogRequest::Level level);
}