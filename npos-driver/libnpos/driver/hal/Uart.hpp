#pragma once

/**
 * @file Uart.hpp
 * @author Adrian Szczepanski
 * @date 07-09-2026
 */

#include <libnpos/driver/hal/Interrupt.hpp>
#include <libnpos/driver/message/Types.hpp>

namespace npos::driver::hal
{
    enum UartType
    {
        UART_2 = 0x00,
    };

    class UartInterrupt : public Interruptable
    {
    public:

        struct Interrupt : public etl::message<message::INTERRUPT_UART>
        {
            enum Event
            {
                TRANSMIT_COMPLETE,
                RECEIVE_COMPLETE,
                ERROR,
            };

            UartType type;
            Event event;
        };

        virtual ~UartInterrupt() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;
        virtual UartType getType() const = 0;

        virtual bool transmitIt(const uint8_t* data, size_t size) = 0;
        virtual bool receiveIt(uint8_t* data, size_t size) = 0;
    };

    class UartPolling
    {
    public:
        virtual ~UartPolling() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;
        virtual UartType getType() const = 0;

        virtual bool transmit(const uint8_t* data, size_t size) = 0;
        virtual bool receive(uint8_t* data, size_t size) = 0;
        virtual size_t receiveAvailable() const = 0;
        virtual size_t transmitAvailable() const = 0;
    };
}