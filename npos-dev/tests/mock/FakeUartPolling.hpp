#pragma once

/**
 * @file FakeUartPolling.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/Uart.hpp>

namespace mock
{
    class FakeUartPolling : public npos::dev::hal::UartPolling
    {
    public:
        bool init() override { initCount++; return not failInit; }
        void deinit() override {}

        bool transmit(const uint8_t* data, size_t size) override
        {
            transmitted = data;
            transmittedSize = size;
            return not failTransmit;
        }

        bool receive(uint8_t* data, size_t size) override
        {
            received = data;
            receivedSize = size;
            return not failReceive;
        }

        size_t receiveAvailable() const override { return 0; }
        size_t transmitAvailable() const override { return 0; }

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
