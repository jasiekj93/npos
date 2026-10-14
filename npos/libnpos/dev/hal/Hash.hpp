#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-09-2026
 */

#include <etl/span.h>

#include <libnpos/dev/hal/Interrupt.hpp>

namespace npos::dev::hal
{
    class Hash : public Interruptable
    {
    public:
        static constexpr size_t DIGEST_SIZE = 32; 

        enum Event : Interrupt::Code
        {
            INPUT_COMPLETE = 0x00,
            DIGEST_COMPLETE = 0x01,
            ERROR = 0x02
        };


        virtual ~Hash() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;

        virtual bool accumulateIt(etl::span<const uint8_t> input) = 0;
        virtual bool accumulateEndIt(etl::span<const uint8_t> input, etl::span<uint8_t> output) = 0;
    };
}