#pragma once

/**
 * @file Watchdog.hpp
 * @author Adrian Szczepanski
 * @date 10-09-2026
 */

#include <cstddef>

namespace npos::driver::hal
{
    class Watchdog
    {
    public:
        virtual ~Watchdog() = default;
        
        virtual bool init() = 0;
        virtual void refresh() = 0;
    };
}