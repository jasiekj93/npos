#pragma once

/**
 * @file Watchdog.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <iostream>

#include <libnpos/dev/hal/Watchdog.hpp>

namespace npos::emulator
{
    class Watchdog : public npos::dev::hal::Watchdog
    {
    public:
        explicit Watchdog(bool verbose = false) : verbose(verbose) {}

        bool init() override
        {
            initialized = true;

            if(verbose)
                std::cout << "Emulator: Watchdog init" << std::endl;

            return true;
        }

        void refresh() override { refreshCount++; }

        bool verbose;
        bool initialized = false;
        int refreshCount = 0;
    };
}
