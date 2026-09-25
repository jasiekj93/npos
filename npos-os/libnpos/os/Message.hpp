#pragma once

/**
 * @file Message.hpp
 * @author Adrian Szczepanski
 * @date 18-09-2026
 */

#include <cstdint>
#include <cstddef>

namespace npos::os
{
    using Pid = uint8_t; 

    struct Message 
    {
        static constexpr size_t PAYLOAD_SIZE = 128;

        using Type = uint16_t;
        using ObjectId = uint8_t;
        using Priority = uint8_t;
        using ReferenceCount = uint8_t;
        using Status = uint8_t;

        struct OpenClose
        {
            using Flags = int;

            Flags flags;
        };

        struct InputOutput
        {
            using Mode = uint8_t;

            size_t offset;
            size_t length;
            Mode mode;
        };

        struct Attributes
        {
            uint64_t value;
            int type;
        };

        struct Create
        {
            ObjectId object;
        };

        Type type;
        Pid sender;
        ObjectId object;

        Priority priority;
        ReferenceCount referenceCount;

        Status status;

        union
        {
            OpenClose openClose;
            InputOutput inputOutput;
            Attributes attributes;
            Create create;

            uint8_t payload[PAYLOAD_SIZE];
        };

        size_t size;
        uint8_t* data;
    };

    struct CompareMessage : public etl::binary_function<Message, Message, bool>
    {
        bool operator()(const Message& lhs, const Message& rhs) const
        {
            return lhs.priority < rhs.priority;
        }
    };

    static constexpr Message::Type OS_ERROR_MESSAGE_TYPE = 0x0000;
    static constexpr Message::Type OS_SYSLOG_MESSAGE_TYPE = 0x0001;
    static constexpr Message::Type DEVICE_MESSAGE_TYPE = 0x0200;
    static constexpr Message::Type DRIVER_MESSAGE_TYPE = 0x0300;
    static constexpr Message::Type FILESYSTEM_MESSAGE_TYPE = 0x0400;
    static constexpr Message::Type CLI_MESSAGE_TYPE = 0x0500;
    static constexpr Message::Type USER_MESSAGE_TYPE = 0x0600;
}