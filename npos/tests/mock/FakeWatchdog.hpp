#pragma once

/**
 * @file FakeWatchdog.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Watchdog.hpp>

namespace mock
{
    class FakeWatchdog : public npos::dev::hal::Watchdog
    {
    public:
        bool init() override { initCount++; return not failInit; }
        void refresh() override { refreshCount++; }

        bool failInit = false;
        int initCount = 0;
        int refreshCount = 0;
    };
}
