#pragma once

/**
 * @file Uart.hpp
 * @author Adrian Szczepanski
 * @date 07-09-2026
 */

#include <cstddef>

#include <libnpos/driver/hal/Interrupt.hpp>

namespace npos::driver::hal
{
    class UartInterrupt : public Interruptable
    {
    public:
        enum Event : Interrupt::Code
        {
            TRANSMIT_COMPLETE = 0,
            RECEIVE_COMPLETE,
            ERROR,
        };

        virtual ~UartInterrupt() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;

        virtual bool transmitIt(const uint8_t* data, size_t size) = 0;
        virtual bool receiveIt(uint8_t* data, size_t size) = 0;
    };

    class UartPolling
    {
    public:
        virtual ~UartPolling() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;

        virtual bool transmit(const uint8_t* data, size_t size) = 0;
        virtual bool receive(uint8_t* data, size_t size) = 0;
        virtual size_t receiveAvailable() const = 0; // ?
        virtual size_t transmitAvailable() const = 0;
    };
}