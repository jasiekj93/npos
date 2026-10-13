/**
 * @file Power.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/Power.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorPower)
{
    npos::emulator::Power power;
};

TEST(EmulatorPower, CountsSleeps)
{
    power.enterSleepMode();
    power.enterSleepMode();

    CHECK_EQUAL(2, power.sleepCount);
}
