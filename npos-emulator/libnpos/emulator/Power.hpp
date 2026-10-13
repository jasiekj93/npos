#pragma once

/**
 * @file Power.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <iostream>

#include <libnpos/dev/hal/Power.hpp>

namespace npos::emulator
{
    class Power : public npos::dev::hal::Power
    {
    public:
        explicit Power(bool verbose = false) : verbose(verbose) {}

        void enterSleepMode() override
        {
            sleepCount++;

            if(verbose)
                std::cout << "Emulator: Power sleep" << std::endl;
        }

        bool verbose;
        int sleepCount = 0;
    };
}
