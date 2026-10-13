/**
 * @file UartPolling.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/UartPolling.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorUartPolling)
{
    npos::emulator::UartPolling uart;
    uint8_t buffer[4] = { 1, 1, 1, 1 };
};

TEST(EmulatorUartPolling, TransfersFailBeforeInit)
{
    CHECK_FALSE(uart.transmit(buffer, sizeof(buffer)));
    CHECK_FALSE(uart.receive(buffer, sizeof(buffer)));
}

TEST(EmulatorUartPolling, TransmitCountsBytes)
{
    uart.init();

    CHECK_TRUE(uart.transmit(buffer, 3));
    CHECK_TRUE(uart.transmit(buffer, 2));

    CHECK_EQUAL(5, uart.transmitted);
}

TEST(EmulatorUartPolling, ReceiveFillsZeros)
{
    uart.init();

    CHECK_TRUE(uart.receive(buffer, sizeof(buffer)));

    CHECK_EQUAL(0, buffer[0] + buffer[3]);
}

TEST(EmulatorUartPolling, DeinitDisablesTransfers)
{
    uart.init();
    uart.deinit();

    CHECK_FALSE(uart.transmit(buffer, 1));
    CHECK_EQUAL(0, uart.receiveAvailable());
    CHECK_TRUE(uart.transmitAvailable() > 0);
}
