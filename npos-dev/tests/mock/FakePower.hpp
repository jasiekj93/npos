#pragma once

/**
 * @file FakePower.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Power.hpp>

namespace mock
{
    class FakePower : public npos::dev::hal::Power
    {
    public:
        void enterSleepMode() override { sleepCount++; }

        int sleepCount = 0;
    };
}
