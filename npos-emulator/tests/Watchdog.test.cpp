/**
 * @file Watchdog.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/Watchdog.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorWatchdog)
{
    npos::emulator::Watchdog watchdog;
};

TEST(EmulatorWatchdog, InitSucceeds)
{
    CHECK_TRUE(watchdog.init());
    CHECK_TRUE(watchdog.initialized);
}

TEST(EmulatorWatchdog, CountsRefreshes)
{
    watchdog.refresh();
    watchdog.refresh();

    CHECK_EQUAL(2, watchdog.refreshCount);
}
