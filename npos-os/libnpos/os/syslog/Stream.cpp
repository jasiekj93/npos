#include "Stream.hpp"

using namespace npos;
using namespace npos::os;
using namespace npos::os::syslog;

static Level currentLevel;

Stream::Stream(os::Message::SenderId senderId, Level level, Stream::SystemBus& bus)
    : bus(bus)
    , stream(request.message)
{
    request.senderId = senderId;
    request.level = level;
    request.senderId = senderId;
}

Stream& Stream::operator<<(etl::string_view str) 
{ 
    if(request.level > currentLevel or request.overflowFlag)
        return *this;

    if(request.message.available() < str.size())
        request.overflowFlag = true;
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
    if(request.level > currentLevel or request.overflowFlag)
        return *this; 

    if(request.message.available() < span.size() * 2)
        request.overflowFlag = true;
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
    if(request.level > currentLevel or request.overflowFlag)
        return *this;

    if(request.message.available() < 5) // "true" or "false" length
        request.overflowFlag = true;
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
    if(request.level > currentLevel or request.overflowFlag)
        return *this; 

    if(request.message.available() < 2)
        request.overflowFlag = true;
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
    if(request.level > currentLevel or request.overflowFlag)
        return *this;

    stream.set_format(format);
    return *this; 
}

inline Stream& Stream::operator<<(const Endl&) 
{ 
    if(request.level > currentLevel)
        return *this;

    bus.receive(request);

    request.message.clear();
    request.overflowFlag = false;
    return *this;
}

Stream npos::os::syslog::log(os::Message::SenderId senderId, Level level, Stream::SystemBus& bus)
{
    return Stream(senderId, level, bus);
}

void npos::os::syslog::setLevel(Level level)
{
    currentLevel = level;
}