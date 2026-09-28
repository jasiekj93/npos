#pragma once

/**
 * @file Interruptable.hpp
 * @author Adrian Szczepanski
 * @date 16-09-2026
 */

#include <cstdint>

namespace npos::driver::hal
{
    struct Interrupt
    {
        enum Type
        {
            HASH = 0,
            UART,
            WATCHDOG
        };

        using Code = uint8_t;

        Type type;
        Code code;
    };

    class InterruptHandler
    {
    public:
        virtual ~InterruptHandler() = default;

        virtual void handleInterrupt(const Interrupt&) = 0;
    };

    class Interruptable
    {
    public:
        virtual ~Interruptable() = default;

        virtual void enableInterrupts() = 0;
        virtual void disableInterrupts() = 0;
        virtual void setHandler(InterruptHandler*) = 0;
    };
}