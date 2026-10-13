/**
 * @file Idle.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakePower.hpp"

#include <libnpos/dev/task/Idle.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Idle)
{
    mock::FakePower power;
    npos::dev::task::Idle idle{power};
};

TEST(Idle, HasLowestPriority)
{
    CHECK_EQUAL(0, idle.getPriority());
}

TEST(Idle, IsAlwaysReady)
{
    CHECK_TRUE(idle.isReady());
}

TEST(Idle, ProcessEntersSleepMode)
{
    idle.process();
    idle.process();

    CHECK_EQUAL(2, power.sleepCount);
}
