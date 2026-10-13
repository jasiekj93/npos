#pragma once

/**
 * @file FakeSystemTick.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/SystemTick.hpp>

namespace mock
{
    class FakeSystemTick : public npos::dev::hal::SystemTick
    {
    public:
        npos::dev::Tick getTick() const override { return tick; }

        npos::dev::Tick tick = 0;
    };
}
