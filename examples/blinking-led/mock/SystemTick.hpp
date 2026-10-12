#pragma once

/**
 * @file SystemTick.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <chrono>

#include <libnpos/dev/hal/SystemTick.hpp>

namespace mock
{
    class SystemTick : public npos::dev::hal::SystemTick
    {
    public:
        npos::dev::Tick getTick() const override
        {
            return static_cast<npos::dev::Tick>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count()
            );
        }
    };
}