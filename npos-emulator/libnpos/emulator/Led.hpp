#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <iostream>

#include <libnpos/dev/hal/Led.hpp>

namespace npos::emulator
{
    class Led : public npos::dev::hal::Led
    {
    public:
        explicit Led(bool verbose = false) : verbose(verbose) {}

        void toggle() override
        {
            state = not state;
            printState();
        }

        void on() override
        {
            state = true;
            printState();
        }

        void off() override
        {
            state = false;
            printState();
        }

        void printState()
        {
            if(verbose)
                std::cout << "Emulator: LED state: " << (state ? "ON" : "OFF") << std::endl;
        }

        bool state = false;
        bool verbose;
    };
}
