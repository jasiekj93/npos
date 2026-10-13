/**
 * @file Tick.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/Tick.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Tick)
{
};

TEST(Tick, IsThirtyTwoBitUnsigned)
{
    CHECK_EQUAL(4, sizeof(npos::dev::Tick));
    CHECK_TRUE(static_cast<npos::dev::Tick>(-1) > 0);
}
