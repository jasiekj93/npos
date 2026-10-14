#pragma once

/**
 * @file FakeHash.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "InterruptSource.hpp"

#include <libnpos/dev/hal/Hash.hpp>

namespace mock
{
    class FakeHash : public npos::dev::hal::Hash
    {
    public:
        bool init() override { initCount++; return not failInit; }
        void deinit() override {}

        bool accumulateIt(etl::span<const uint8_t> input) override
        {
            accumulated = input.size();
            return not failAccumulate;
        }

        bool accumulateEndIt(etl::span<const uint8_t> input, etl::span<uint8_t> output) override
        {
            accumulated = input.size();
            digestSize = output.size();
            return not failAccumulate;
        }

        void enableInterrupts() override { source.enableCount++; }
        void disableInterrupts() override { source.disableCount++; }
        void setHandler(npos::dev::hal::InterruptHandler* newHandler) override { source.handler = newHandler; }

        void fire(npos::dev::hal::Interrupt::Type type, npos::dev::hal::Interrupt::Code code) { source.fire(type, code); }

        InterruptSource source;
        bool failInit = false;
        bool failAccumulate = false;
        int initCount = 0;
        size_t accumulated = 0;
        size_t digestSize = 0;
    };
}
