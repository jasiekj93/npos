#pragma once

/**
 * @file FakeICache.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/hal/ICache.hpp>

namespace mock
{
    class FakeICache : public npos::fs::hal::ICache
    {
    public:
        bool init() override { initCount++; return true; }
        bool enable() override { enableCount++; return not failEnable; }
        bool disable() override { disableCount++; return not failDisable; }

        bool failEnable = false;
        bool failDisable = false;
        int initCount = 0;
        int enableCount = 0;
        int disableCount = 0;
    };
}
