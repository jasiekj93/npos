#pragma once

/**
 * @file FakeUart.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "InterruptSource.hpp"

#include <libnpos/dev/hal/Uart.hpp>

namespace mock
{
    class FakeUart : public npos::dev::hal::UartInterrupt
    {
    public:
        bool init() override { initCount++; return not failInit; }
        void deinit() override {}

        bool transmitIt(const uint8_t* data, size_t size) override
        {
            transmitted = data;
            transmittedSize = size;
            return not failTransmit;
        }

        bool receiveIt(uint8_t* data, size_t size) override
        {
            received = data;
            receivedSize = size;
            return not failReceive;
        }

        void enableInterrupts() override { source.enableCount++; }
        void disableInterrupts() override { source.disableCount++; }
        void setHandler(npos::dev::hal::InterruptHandler* newHandler) override { source.handler = newHandler; }

        void fire(npos::dev::hal::Interrupt::Type type, npos::dev::hal::Interrupt::Code code, npos::dev::hal::Interrupt::Value value = 0) { source.fire(type, code, value); }

        InterruptSource source;
        bool failInit = false;
        bool failTransmit = false;
        bool failReceive = false;
        int initCount = 0;
        const uint8_t* transmitted = nullptr;
        size_t transmittedSize = 0;
        uint8_t* received = nullptr;
        size_t receivedSize = 0;
    };
}
