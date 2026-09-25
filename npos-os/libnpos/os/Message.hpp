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
        static constexpr size_t RAW_SIZE = 32;

        using Type = uint16_t;
        using ObjectId = uint8_t;
        using Priority = uint8_t;
        using ReferenceCount = uint8_t;

        struct Input
        {
            struct OpenClose
            {
                int flags;
            };

            struct InputOutput
            {
                size_t offset;
                size_t length;
                unsigned mode;
            };

            struct Attributes
            {
                uint64_t value;
                int type;
            };

            union
            {
                OpenClose openClose;
                InputOutput inputOutput;
                Attributes attributes;

                uint8_t raw[RAW_SIZE];
            };

            size_t size;
            const uint8_t* data;
        };

        struct Output
        {
            using Status = uint8_t;

            struct Attributes
            {
                uint64_t value;
            };

            struct Create
            {
                ObjectId object;
            };

            union 
            {
                Attributes attributes;
                Create create;
                uint8_t raw[RAW_SIZE];
            };

            Status status;
            size_t size;
            uint8_t* data;
        };

        Type type;
        Pid sender;
        ObjectId object;
        Priority priority;
        ReferenceCount referenceCount;

        Input input;
        Output output;
    };

    struct CompareMessage : public etl::binary_function<Message, Message, bool>
    {
        bool operator()(const Message& lhs, const Message& rhs) const
        {
            return lhs.priority < rhs.priority;
        }
    };
}