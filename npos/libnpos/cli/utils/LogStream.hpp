#pragma once

/**
 * @file LogStream.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <etl/string_stream.h>

#include <libnpos/ipc/Service.hpp>
#include <libnpos/cli/msg/Syslog.hpp>

namespace npos::cli::utils
{
    class LogStream
    {
    public:
        static constexpr size_t MAX_NUMBER_LENGTH = 20; 
        static constexpr size_t MAX_MESSAGE_LENGTH = 128; 

        LogStream(ipc::Service& sender, log::Level level)
            : level(level)
            , sender(sender)
            , stream(buffer)
            , overflowFlag(false)
        {
        }

        LogStream& operator<<(etl::string_view str) 
        { 
            if(overflowFlag)
                return *this;

            if(buffer.available() < str.size())
                overflowFlag = true;
            else
                stream << str;

            return *this; 
        }

        inline LogStream& operator<<(const char* str) 
        { 
            return operator<<(etl::string_view(str));
        }

        inline LogStream& operator<<(const etl::ivector<uint8_t>& vector) 
        { 
            return operator<<(etl::span<const uint8_t>(vector.data(), vector.size()));
        }

        inline LogStream& operator<<(const etl::span<const uint8_t>& span) 
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

        inline LogStream& operator<<(bool value) 
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

        inline LogStream& operator<<(int value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline LogStream& operator<<(uint8_t value) 
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

        inline LogStream& operator<<(unsigned int value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline LogStream& operator<<(unsigned long value) 
        { 
            etl::to_string(value, numberBuffer);
            return operator<<(numberBuffer);
        }

        inline LogStream& operator<<(const etl::format_spec& format) 
        { 
            if(overflowFlag)
                return *this;

            stream.set_format(format);
            return *this; 
        }

        inline LogStream& operator<<(const log::Endl&) 
        { 
            msg::Syslog request(sender.getOid(), level, buffer, overflowFlag);
            sender.getApi().send(sender, request);

            buffer.clear();
            overflowFlag = false;
            return *this;
        }

    
    private:
        log::Level level;
        ipc::Service& sender;
        etl::string<MAX_MESSAGE_LENGTH> buffer;
        etl::string<MAX_NUMBER_LENGTH> numberBuffer;
        etl::string_stream stream;
        bool overflowFlag;
    };
}