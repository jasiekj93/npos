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
        static constexpr size_t RAW_SIZE = 16;

        using Status = uint8_t;
        using Type = uint16_t;
        using ReferenceCount = uint8_t;

        static constexpr Status STATUS_MESSAGE_NOT_SUPPORTED = 0xFF;
        static constexpr Status STATUS_PORT_NOT_FOUND = 0xFE;
        static constexpr Status STATUS_SERVICE_NOT_FOUND = 0xFD;

        struct Input
        {
            struct Open
            {
                using Flags = uint32_t;

                Flags flags;
            };

            struct Attributes
            {
                using Value = uint64_t;
                using Mode = uint8_t;

                Mode mode;
                Value value;
            };

            struct ReadWrite
            {
                using Offset = uint32_t;
                using Length = size_t;
                using Mode = uint8_t;

                Offset offset;
                Length length;
                Mode mode;
            };

            struct Link
            {
                Oid target;
                Attributes attributes;
            };

            union
            {
                Open open;
                Attributes attributes;
                ReadWrite readWrite;
                Link link;

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

            struct ReadWrite
            {
                using Length = size_t;

                Length length;
            };

            union
            {
                Attributes attributes;
                Open open;
                ReadWrite readWrite;

                uint8_t raw[RAW_SIZE];
            };

            Status status = STATUS_MESSAGE_NOT_SUPPORTED;
            uint8_t* data;
            size_t size;
        };

        Oid sender = NULL_OID;
        Oid recipient = NULL_OID;
        ReferenceCount referenceCount = 0;
        Type type;

        Input input;
        Output output;
    };

    static constexpr Message::Type OS_ERROR_MESSAGE_TYPE = 0x0000;
    static constexpr Message::Type DEVICE_MESSAGE_TYPE = 0x0100;
    static constexpr Message::Type DRIVER_MESSAGE_TYPE = 0x0200;
    static constexpr Message::Type FILESYSTEM_MESSAGE_TYPE = 0x0300;
    static constexpr Message::Type CLI_MESSAGE_TYPE = 0x0400;
    static constexpr Message::Type USER_MESSAGE_TYPE = 0x0500;
}
