/**
 * @file SystemTick.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/SystemTick.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorSystemTick)
{
    npos::emulator::SystemTick tick;
};

TEST(EmulatorSystemTick, TickDoesNotGoBackwards)
{
    auto first = tick.getTick();
    auto second = tick.getTick();

    CHECK_TRUE(second >= first);
}
