#pragma once

/**
 * @file FakeLed.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Led.hpp>

namespace mock
{
    class FakeLed : public npos::dev::hal::Led
    {
    public:
        void on() override { state = true; onCount++; }
        void off() override { state = false; offCount++; }
        void toggle() override { state = not state; toggleCount++; }

        bool state = false;
        int onCount = 0;
        int offCount = 0;
        int toggleCount = 0;
    };
}
