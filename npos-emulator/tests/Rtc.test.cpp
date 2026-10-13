/**
 * @file Rtc.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/Rtc.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorRtc)
{
    npos::emulator::Rtc rtc;
};

TEST(EmulatorRtc, InitializesSuccessfully)
{
    CHECK_TRUE(rtc.initalize());
}

TEST(EmulatorRtc, StartsAtFixedDateAndTime)
{
    npos::dev::hal::Rtc::Time time;
    npos::dev::hal::Rtc::Date date;

    CHECK_TRUE(rtc.getTime(time));
    CHECK_TRUE(rtc.getDate(date));

    CHECK_EQUAL(12, time.hours);
    CHECK_EQUAL(50, time.minutes);
    CHECK_EQUAL(24, time.seconds);
    CHECK_EQUAL(29, date.day);
    CHECK_EQUAL(9, date.month);
    CHECK_EQUAL(26, date.year);
}

TEST(EmulatorRtc, KeepsWrittenTimeAndDate)
{
    npos::dev::hal::Rtc::Time time { 1, 2, 3 }, readTime;
    npos::dev::hal::Rtc::Date date { 4, 5, 6 }, readDate;

    CHECK_TRUE(rtc.setTime(time));
    CHECK_TRUE(rtc.setDate(date));
    rtc.getTime(readTime);
    rtc.getDate(readDate);

    CHECK_EQUAL(3, readTime.seconds);
    CHECK_EQUAL(5, readDate.month);
}
