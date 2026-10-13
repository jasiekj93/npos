/**
 * @file Timestamp.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/Timestamp.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Timestamp)
{
};

TEST(Timestamp, IsSixtyFourBitUnsigned)
{
    CHECK_EQUAL(8, sizeof(npos::dev::Timestamp));
    CHECK_TRUE(static_cast<npos::dev::Timestamp>(-1) > 0);
}
