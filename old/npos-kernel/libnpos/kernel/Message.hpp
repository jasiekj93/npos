#pragma once

/**
 * @file Message.hpp
 * @author Adrian Szczepanski
 * @date 18-09-2026
 */

#include <cstdint>
#include <cstddef>

#include <etl/functional.h>

#include <libnpos/kernel/Oid.hpp>

namespace npos::kernel
{
    struct Message 
    {
        static constexpr size_t RAW_SIZE = 32;

        using Type = uint16_t;
        using ReferenceCount = uint8_t;
        using Status = uint8_t;

        struct Input
        {
            struct OpenClose
            {
                using Flags = int;

                Flags flags;
            };

            struct IO
            {
                using Mode = uint8_t;

                size_t offset;
                size_t length;
                Mode mode;
            };

            struct Attributes
            {
                using Type = uint32_t;
                using Value = uint64_t;

                Value value;
                Type type;
            };

            struct Syslog
            {
                using Level = uint8_t;

                Level level;
                bool overflow;
            };

            union
            {
                OpenClose openClose;
                IO io;
                Attributes attributes;
                Syslog syslog;

                uint8_t raw[RAW_SIZE];
            };

            size_t size = 0;
            const uint8_t* data = nullptr;
        };

        struct Output
        {
            struct Attributes
            {
                using Value = uint64_t;

                Value value;
            };

            struct Create
            {
                Oid object;
            };

            union
            {
                Attributes attributes;
                Create create;

                uint8_t raw[RAW_SIZE];
            };

            Status status = 0;
            size_t size = 0;
            uint8_t* data = nullptr;
        };

        Type type;
        PortId sender;
        Oid object;
        ReferenceCount referenceCount = 0;

        Input input;
        Output output;
    };

    static constexpr Message::Type OS_ERROR_MESSAGE_TYPE = 0x0000;
    static constexpr Message::Type OS_SYSLOG_MESSAGE_TYPE = 0x0001;
    static constexpr Message::Type DEVICE_MESSAGE_TYPE = 0x0200;
    static constexpr Message::Type DRIVER_MESSAGE_TYPE = 0x0300;
    static constexpr Message::Type FILESYSTEM_MESSAGE_TYPE = 0x0400;
    static constexpr Message::Type CLI_MESSAGE_TYPE = 0x0500;
    static constexpr Message::Type USER_MESSAGE_TYPE = 0x0600;
}