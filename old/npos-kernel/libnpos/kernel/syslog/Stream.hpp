#pragma once

/**
 * @file Stream.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/span.h>
#include <etl/vector.h>
#include <etl/string_stream.h>

#include <libnpos/kernel/Bus.hpp>
#include <libnpos/kernel/syslog/Request.hpp>

namespace npos::kernel::syslog
{
    struct Endl {};
    static Endl endl;

    class Stream
    {
    public:
        static constexpr size_t MAX_MESSAGE_SIZE = 128;
        static constexpr size_t MAX_NUMBER_LENGTH = 20; // max length of long in decimal

        Stream(kernel::Pid senderId, Level level, Bus& bus);

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
        Bus& bus;
        etl::string<MAX_NUMBER_LENGTH> numberBuffer;
        etl::string<MAX_MESSAGE_SIZE> buffer;
        etl::string_stream stream;
        bool overflowFlag;
        Level level;
        kernel::Pid senderId;
    };

    Stream log(kernel::Pid senderId, Level level, Bus& bus);
    void setLevel(Level level);
}