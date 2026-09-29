#pragma once

/**
 * @file Message.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <cstddef>
#include <cstdint>

#include <libnpos/ipc/Oid.hpp>

namespace npos::ipc
{
    struct Message
    {
        static constexpr size_t RAW_SIZE = 32;

        using Status = uint8_t;
        using Type = uint16_t;

        struct Input
        {
            struct Attributes
            {
                using Value = uint64_t;
                using Mode = uint8_t;

                Mode mode;
                Value value;
            };

            union
            {
                Attributes attributes;

                uint8_t raw[RAW_SIZE];
            };
            

            const uint8_t* data;
            size_t size;
        };

        struct Output
        {
            struct Attributes
            {
                using Value = uint64_t;
                
                Value value;
            };

            struct Open
            {
                Oid opened;
            };

            union
            {
                Attributes attributes;
                Open open;

                uint8_t raw[RAW_SIZE];
            };

            Status status;
            uint8_t* data;
            size_t size;
        };

        PortId sender = NULL_PORT;
        Oid recipient = NULL_OID;
        Type type;

        Input input;
        Output output;
    };

    static constexpr Message::Type OS_ERROR_MESSAGE_TYPE = 0x0000;
    static constexpr Message::Type OS_SYSLOG_MESSAGE_TYPE = 0x0001;
    static constexpr Message::Type DEVICE_MESSAGE_TYPE = 0x0100;
    static constexpr Message::Type DRIVER_MESSAGE_TYPE = 0x0200;
    static constexpr Message::Type FILESYSTEM_MESSAGE_TYPE = 0x0300;
    static constexpr Message::Type CLI_MESSAGE_TYPE = 0x0400;
    static constexpr Message::Type USER_MESSAGE_TYPE = 0x0500;
}
