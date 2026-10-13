#pragma once

/**
 * @file UartPolling.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstring>
#include <iostream>

#include <libnpos/dev/hal/Uart.hpp>

namespace npos::emulator
{
    // Polling uart that accepts everything it is given and receives zeros.
    class UartPolling : public npos::dev::hal::UartPolling
    {
    public:
        explicit UartPolling(bool verbose = false) : verbose(verbose) {}

        bool init() override
        {
            initialized = true;
            return true;
        }

        void deinit() override { initialized = false; }

        bool transmit(const uint8_t*, size_t size) override
        {
            if(not initialized)
                return false;

            transmitted += size;

            if(verbose)
                std::cout << "Emulator: UartPolling transmit, size: " << size << std::endl;

            return true;
        }

        bool receive(uint8_t* data, size_t size) override
        {
            if(not initialized)
                return false;

            std::memset(data, 0, size);

            if(verbose)
                std::cout << "Emulator: UartPolling receive, size: " << size << std::endl;

            return true;
        }

        size_t receiveAvailable() const override { return 0; }
        size_t transmitAvailable() const override { return 1024; }

        bool verbose;
        bool initialized = false;
        size_t transmitted = 0;
    };
}
