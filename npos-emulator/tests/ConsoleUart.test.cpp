/**
 * @file ConsoleUart.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/vector.h>

#include <libnpos/emulator/ConsoleUart.hpp>

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

TEST_GROUP(EmulatorConsoleUart)
{
    npos::emulator::ConsoleUart uart;
    Handler handler;
    uint8_t buffer[1] = { 0 };

    void setup() override { uart.setHandler(&handler); }
};

TEST(EmulatorConsoleUart, TransfersFailBeforeInit)
{
    CHECK_FALSE(uart.transmitIt(buffer, 0));
    CHECK_FALSE(uart.receiveIt(buffer, sizeof(buffer)));
}

TEST(EmulatorConsoleUart, InitSucceeds)
{
    CHECK_TRUE(uart.init());
    CHECK_TRUE(uart.initialized);
}

TEST(EmulatorConsoleUart, TransmitRaisesTransmitCompleteInterrupt)
{
    uart.init();

    CHECK_TRUE(uart.transmitIt(buffer, 0));

    CHECK_EQUAL(1, handler.events.size());
    CHECK_EQUAL(npos::dev::hal::UartInterrupt::TRANSMIT_COMPLETE, handler.events[0]);
}

TEST(EmulatorConsoleUart, TransmitWorksWithoutHandler)
{
    uart.setHandler(nullptr);
    uart.init();

    CHECK_TRUE(uart.transmitIt(buffer, 0));
}

TEST(EmulatorConsoleUart, InterruptControlAndDeinitAreCallable)
{
    uart.enableInterrupts();
    uart.disableInterrupts();
    uart.deinit();

    CHECK_EQUAL(0, handler.events.size());
}
