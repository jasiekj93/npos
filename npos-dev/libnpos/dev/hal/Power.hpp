#pragma once

/**
 * @file Power.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

namespace npos::dev::hal
{
    class Power
    {
    public:
        virtual ~Power() = default;

        virtual void enterSleepMode() = 0;
    };
}