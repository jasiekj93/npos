#pragma once

/**
 * @file InterruptSource.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Interrupt.hpp>

namespace mock
{
    class InterruptSource
    {
    public:
        void fire(npos::dev::hal::Interrupt::Type type, npos::dev::hal::Interrupt::Code code)
        {
            if(handler)
                handler->handleInterrupt({ type, code });
        }

        npos::dev::hal::InterruptHandler* handler = nullptr;
        int enableCount = 0;
        int disableCount = 0;
    };
}
