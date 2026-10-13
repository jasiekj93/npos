/**
 * @file UartInterrupt.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/emulator/UartInterrupt.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Handler : public npos::dev::hal::InterruptHandler
    {
    public:
        void handleInterrupt(const npos::dev::hal::Interrupt& interrupt) override { events.push_back(interrupt.code); }

        etl::vector<int, 4> events;
    };
}

TEST_GROUP(EmulatorUartInterrupt)
{
    npos::emulator::UartInterrupt uart;
    Handler handler;
    uint8_t buffer[4] = {};

    void setup() override { uart.setHandler(&handler); }
};

TEST(EmulatorUartInterrupt, TransfersFailBeforeInit)
{
    CHECK_FALSE(uart.transmitIt(buffer, sizeof(buffer)));
    CHECK_FALSE(uart.receiveIt(buffer, sizeof(buffer)));
}

TEST(EmulatorUartInterrupt, TransmitRaisesTransmitCompleteInterrupt)
{
    uart.init();

    CHECK_TRUE(uart.transmitIt(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::dev::hal::UartInterrupt::TRANSMIT_COMPLETE, handler.events[0]);
}

TEST(EmulatorUartInterrupt, ReceiveRaisesReceiveCompleteInterrupt)
{
    uart.init();

    CHECK_TRUE(uart.receiveIt(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::dev::hal::UartInterrupt::RECEIVE_COMPLETE, handler.events[0]);
}

TEST(EmulatorUartInterrupt, WorksWithoutHandler)
{
    uart.setHandler(nullptr);
    uart.init();

    CHECK_TRUE(uart.transmitIt(buffer, sizeof(buffer)));
}

TEST(EmulatorUartInterrupt, InterruptControlAndDeinitAreCallable)
{
    uart.enableInterrupts();
    uart.disableInterrupts();
    uart.deinit();

    CHECK_EQUAL(0, handler.events.size());
}
