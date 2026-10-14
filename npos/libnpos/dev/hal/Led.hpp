#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <cstdint>

namespace npos::dev::hal
{
    class Led
    {
    public:
        virtual ~Led() = default;

        virtual void on() = 0;
        virtual void off() = 0;
        virtual void toggle() = 0;
    };
}