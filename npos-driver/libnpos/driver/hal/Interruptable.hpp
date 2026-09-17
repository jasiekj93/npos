#pragma once

/**
 * @file Interruptable.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <libnpos/os/message/Bus.hpp>

namespace npos::os::driver::hal
{
    class Interruptable
    {
    public:
        virtual ~Interruptable() = default;

        virtual void enableInterrupts() = 0;
        virtual void disableInterrupts() = 0;
        virtual void setBus(message::Bus&) = 0;
    };
}