#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <cstdint>

namespace npos::os::driver::hal
{
    class Led
    {
    public:
        using Color = uint8_t;

        virtual ~Led() = default;

        virtual void on(Color) = 0;
        virtual void off(Color) = 0;
        virtual void toggle(Color) = 0;

        virtual bool available(Color) = 0;
    };
}