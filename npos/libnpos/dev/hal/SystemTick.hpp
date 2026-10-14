#pragma once

/**
 * @file SystemTick.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/dev/Tick.hpp>

namespace npos::dev::hal
{
    class SystemTick
    {
    public:
        virtual ~SystemTick() = default;

        virtual Tick getTick() const = 0;
    };
}