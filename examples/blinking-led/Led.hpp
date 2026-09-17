#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <iostream>

#include <libnpos/os/driver/hal/Led.hpp>

class Led : public npos::os::driver::hal::Led
{
    void on(Color color) override
    {
        std::cout << "EMULATOR: Turning on LED with color: " << static_cast<int>(color) << std::endl;
    }
    void off(Color color) override
    {
        std::cout << "EMULATOR: Turning off LED with color: " << static_cast<int>(color) << std::endl;
    }
    void toggle(Color color) override
    {
        std::cout << "EMULATOR: Toggling LED with color: " << static_cast<int>(color) << std::endl;
    }

    bool available(Color color) override
    {
        std::cout << "EMULATOR: Checking availability of LED with color: " << static_cast<int>(color) << std::endl;
        return true;
    }
};