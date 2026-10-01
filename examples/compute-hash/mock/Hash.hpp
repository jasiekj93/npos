#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <iostream>

#include <libnpos/dev/hal/Hash.hpp>

namespace mock
{
    class Hash : public npos::dev::hal::Hash
    {
    public:
        bool init() override
        {
            initialized = true;
            return true;
        }

        void deinit() override
        {
        }

        bool accumulateIt(etl::span<const uint8_t> input) override 
        {
            if(not initialized)
                return false;

            std::cout << "Emulator: Hash accumulateIt, size: " << input.size() << std::endl;

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::HASH, Event::INPUT_COMPLETE});
            return true;
        }

        bool accumulateEndIt(etl::span<const uint8_t> input, etl::span<uint8_t> output) override
        {
            if(not initialized)
                return false;

            if(output.size() < DIGEST_SIZE)
                return false;

            std::cout << "Emulator: Hash accumulateEndIt, input size: " << input.size() << ", output size: " << output.size() << std::endl;

            if(interruptHandler)
                interruptHandler->handleInterrupt({npos::dev::hal::Interrupt::Type::HASH, Event::DIGEST_COMPLETE});
            return true;
        }

        void enableInterrupts() override
        {
            // std::cout << "Emulator: Hash enableInterrupts" << std::endl;
        }

        void disableInterrupts() override
        {
            // std::cout << "Emulator: Hash disableInterrupts" << std::endl;
        }

        void setHandler(npos::dev::hal::InterruptHandler* handler) override
        {
            interruptHandler = handler;
        }

        bool initialized = false;
        npos::dev::hal::InterruptHandler* interruptHandler = nullptr;
    };
}