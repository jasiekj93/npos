/**
 * @file Watchdog.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeWatchdog.hpp"

#include <libnpos/dev/task/Watchdog.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Watchdog)
{
    mock::FakeWatchdog hal;
    npos::dev::task::Watchdog watchdog{hal};
};

TEST(Watchdog, HasLowestPriority)
{
    CHECK_EQUAL(0, watchdog.getPriority());
}

TEST(Watchdog, InitializeStartsHardwareWatchdog)
{
    watchdog.initialize();

    CHECK_EQUAL(1, hal.initCount);
}

TEST(Watchdog, InitializeSurvivesInitFailure)
{
    hal.failInit = true;

    watchdog.initialize();

    CHECK_EQUAL(1, hal.initCount);
}

TEST(Watchdog, IsAlwaysReady)
{
    CHECK_TRUE(watchdog.isReady());
}

TEST(Watchdog, ProcessRefreshesWatchdog)
{
    watchdog.process();
    watchdog.process();

    CHECK_EQUAL(2, hal.refreshCount);
}
