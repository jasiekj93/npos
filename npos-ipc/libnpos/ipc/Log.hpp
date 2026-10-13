#pragma once

/**
 * @file Log.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/string.h>
#include <etl/string_stream.h>
#include <etl/vector.h>
#include <etl/span.h>

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc::log
{
    enum Level : Message::Input::Attributes::Value
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
    };  

    struct Endl {};
    static Endl endl;

    struct Syslog : public ipc::Message
    {
        Syslog(Oid sender, Level level, etl::string_view message, bool overflow)
        {
            this->type = OS_SYSLOG_MESSAGE_TYPE;
            this->immediate = true;
            this->input.link.target = sender;
            this->input.link.attributes.value = level;
            this->input.link.attributes.mode = overflow;
            this->input.data = reinterpret_cast<const uint8_t*>(message.data());
            this->input.size = message.size();
        }

        inline Level getLevel() const
        {
            return static_cast<Level>(this->input.link.attributes.value);
        }

        inline bool getOverflow() const
        {
            return this->input.link.attributes.mode;
        }

        inline etl::string_view getMessage() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline Oid getSender() const
        {
            return this->input.link.target;
        }
    };

    class Stream
    {
    public:
        static constexpr size_t MAX_NUMBER_LENGTH = 20; 
        static constexpr size_t MAX_MESSAGE_LENGTH = 128; 

        class Transmitter
        {
        public:
            virtual ~Transmitter() = default;

            virtual void sendSyslog(Syslog& message) = 0;
        };

        Stream(Level level, Oid sender, Transmitter& transmitter)
            : level(level), sender(sender), transmitter(transmitter), stream(buffer), overflowFlag(false)
        {
        }

        Stream& operator<<(etl::string_view str) 
        { 
            if(overflowFlag)
                return *this;

            if(buffer.available() < str.size())
                overflowFlag = true;
            else
                stream << str;

            return *this; 
        }

        inline Stream& operator<<(const char* str) 
        { 
            return operator<<(etl::string_view(str));
        }

        inline Stream& operator<<(const etl::ivector<uint8_t>& vector) 
        { 
            return operator<<(etl::span<const uint8_t>(vector.data(), vector.size()));
        }

        inline Stream& operator<<(const etl::span<const uint8_t>& span) 
        { 
            if(overflowFlag)
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

        inline Stream& operator<<(bool value) 
        { 
            if(overflowFlag)
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

        inline Stream& operator<<(int value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline Stream& operator<<(uint8_t value) 
        { 
            if(overflowFlag)
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

        inline Stream& operator<<(unsigned int value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline Stream& operator<<(unsigned long value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline Stream& operator<<(const etl::format_spec& format) 
        { 
            if(overflowFlag)
                return *this;

            stream.set_format(format);
            return *this; 
        }

        inline Stream& operator<<(const Endl&) 
        { 
            Syslog request(sender, level, buffer, overflowFlag);
            transmitter.sendSyslog(request);

            buffer.clear();
            overflowFlag = false;
            return *this;
        }

    
    private:
        Level level;
        Oid sender;
        Transmitter& transmitter;
        etl::string<MAX_MESSAGE_LENGTH> buffer;
        etl::string<MAX_NUMBER_LENGTH> numberBuffer;
        etl::string_stream stream;
        bool overflowFlag;
    };
}