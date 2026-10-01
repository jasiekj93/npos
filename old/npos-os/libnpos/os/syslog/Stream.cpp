#include "Stream.hpp"

using namespace npos;
using namespace npos::os;
using namespace npos::os::syslog;

static Level currentLevel;

Stream::Stream(Pid senderId, Level level, Bus& bus)
    : bus(bus)
    , buffer((char*)(&request.payload[1]), MAX_MESSAGE_SIZE)
    , stream(buffer)
{
    request.sender = senderId;
    request.setLevel(level);
}

Stream& Stream::operator<<(etl::string_view str) 
{ 
    if(request.getLevel() > currentLevel or request.hasOverflow())
        return *this;

    if(buffer.available() < str.size())
        request.setOverflow();
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
    if(request.getLevel() > currentLevel or request.hasOverflow())
        return *this; 

    if(buffer.available() < span.size() * 2)
        request.setOverflow();
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
    if(request.getLevel() > currentLevel or request.hasOverflow())
        return *this;

    if(buffer.available() < 5) // "true" or "false" length
        request.setOverflow();
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
    if(request.getLevel() > currentLevel or request.hasOverflow())
        return *this; 

    if(buffer.available() < 2)
        request.setOverflow();
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
    if(request.getLevel() > currentLevel or request.hasOverflow())
        return *this;

    stream.set_format(format);
    return *this; 
}

inline Stream& Stream::operator<<(const Endl&) 
{ 
    if(request.getLevel() > currentLevel)
        return *this;

    request.size = buffer.size();

    bus.broadcast(request);

    buffer.clear();
    request.size = 0;
    request.clearOverflow();
    return *this;
}

Stream npos::os::syslog::log(os::Pid senderId, Level level, Bus& bus)
{
    return Stream(senderId, level, bus);
}

void npos::os::syslog::setLevel(Level level)
{
    currentLevel = level;
}