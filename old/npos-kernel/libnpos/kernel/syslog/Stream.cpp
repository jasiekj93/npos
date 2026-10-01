#include "Stream.hpp"

using namespace npos;
using namespace npos::kernel;
using namespace npos::kernel::syslog;

static Level currentLevel;

Stream::Stream(Pid senderId, Level level, Bus& bus)
    : bus(bus)
    , buffer()
    , stream(buffer)
    , overflowFlag(false)
    , level(level)
    , senderId(senderId)
{
}

Stream& Stream::operator<<(etl::string_view str) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this;

    if(buffer.available() < str.size())
        overflowFlag = true;
    else
        stream << str;

    return *this; 
}

inline Stream& Stream::operator<<(const char* str) 
{ 
    return operator<<(etl::string_view(str));
}

inline Stream& Stream::operator<<(const etl::ivector<uint8_t>& vector) 
{ 
    return operator<<(etl::span<const uint8_t>(vector.data(), vector.size()));
}

inline Stream& Stream::operator<<(const etl::span<const uint8_t>& span) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this; 

    if(buffer.available() < span.size() * 2)
        overflowFlag = true;
    else
    {
        auto format = stream.get_format();
        for(auto byte : span)
        {
            stream  << etl::hex 
                    << etl::setw(2) 
                    << etl::setfill('0') 
                    << (int)byte;  
        }

        stream.set_format(format);
    }

    return *this; 
}

inline Stream& Stream::operator<<(bool value) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this;

    if(buffer.available() < 5) // "true" or "false" length
        overflowFlag = true;
    else
    {
        auto format = stream.get_format();
        stream << etl::boolalpha << value;
        stream.set_format(format);
    }

    return *this; 
}

inline Stream& Stream::operator<<(int value) 
{ 
    etl::to_string(value, numberBuffer);
    return operator<<(numberBuffer);
}

inline Stream& Stream::operator<<(uint8_t value) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this; 

    if(buffer.available() < 2)
        overflowFlag = true;
    else
    {
        auto format = stream.get_format();
        stream  << etl::hex 
                << etl::setw(2) 
                << etl::setfill('0') 
                << (int)value;  

        stream.set_format(format);
    }

    return *this; 
}

inline Stream& Stream::operator<<(unsigned int value) 
{ 
    etl::to_string(value, numberBuffer);
    return operator<<(numberBuffer);
}

inline Stream& Stream::operator<<(unsigned long value) 
{ 
    etl::to_string(value, numberBuffer);
    return operator<<(numberBuffer);
}

inline Stream& Stream::operator<<(const etl::format_spec& format) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this;

    stream.set_format(format);
    return *this; 
}

inline Stream& Stream::operator<<(const Endl&) 
{ 
    if(level > currentLevel or overflowFlag)
        return *this;

    syslog::Request request(senderId, level, buffer, overflowFlag);
    bus.broadcast(request);

    buffer.clear();
    overflowFlag = false;
    return *this;
}

Stream npos::kernel::syslog::log(kernel::Pid senderId, Level level, Bus& bus)
{
    return Stream(senderId, level, bus);
}

void npos::kernel::syslog::setLevel(Level level)
{
    currentLevel = level;
}