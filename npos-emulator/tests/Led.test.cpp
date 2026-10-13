/**
 * @file Led.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/Led.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorLed)
{
    npos::emulator::Led led;
};

TEST(EmulatorLed, IsOffInitially)
{
    CHECK_FALSE(led.state);
}

TEST(EmulatorLed, OnOffAndToggleChangeState)
{
    led.on();
    CHECK_TRUE(led.state);

    led.off();
    CHECK_FALSE(led.state);

    led.toggle();
    CHECK_TRUE(led.state);

    led.toggle();
    CHECK_FALSE(led.state);
}
