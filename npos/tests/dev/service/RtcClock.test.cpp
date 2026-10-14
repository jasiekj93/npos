/**
 * @file RtcClock.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeRtc.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/RtcClock.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::service::RtcClock;
using npos::dev::msg::Clock;

TEST_GROUP(RtcClock)
{
    mock::RecordingApi api;
    mock::FakeRtc rtc;
    RtcClock clock{api, rtc, "clock"};

    void open()
    {
        npos::dev::msg::Open request("clock");
        clock.handle(request);
        api.responded.clear();
    }

    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(RtcClock, ConvertsRtcEpochToTimestamp)
{
    CHECK_EQUAL(946684800, RtcClock::fromRtcTime({ 0, 0, 0 }, { 1, 1, 0 }));
}

TEST(RtcClock, ConvertsRtcDateAndTimeToTimestamp)
{
    CHECK_EQUAL(1790686224, RtcClock::fromRtcTime({ 12, 50, 24 }, { 29, 9, 26 }));
}

TEST(RtcClock, ConvertsLeapDay)
{
    CHECK_EQUAL(1709251199, RtcClock::fromRtcTime({ 23, 59, 59 }, { 29, 2, 24 }));
}

TEST(RtcClock, ConvertsTimestampToRtcTime)
{
    auto result = RtcClock::toRtcTime(1790686224);

    CHECK_TRUE(result.has_value());
    CHECK_EQUAL(12, result->first.hours);
    CHECK_EQUAL(50, result->first.minutes);
    CHECK_EQUAL(24, result->first.seconds);
    CHECK_EQUAL(29, result->second.day);
    CHECK_EQUAL(9, result->second.month);
    CHECK_EQUAL(26, result->second.year);
}

TEST(RtcClock, ConvertsLeapDayTimestamp)
{
    auto result = RtcClock::toRtcTime(1709251199);

    CHECK_TRUE(result.has_value());
    CHECK_EQUAL(29, result->second.day);
    CHECK_EQUAL(2, result->second.month);
    CHECK_EQUAL(24, result->second.year);
}

TEST(RtcClock, AcceptsRangeBoundaries)
{
    CHECK_TRUE(RtcClock::toRtcTime(RtcClock::MIN_TIMESTAMP).has_value());
    CHECK_TRUE(RtcClock::toRtcTime(RtcClock::MAX_TIMESTAMP).has_value());
}

TEST(RtcClock, RejectsTimestampsOutOfRange)
{
    CHECK_FALSE(RtcClock::toRtcTime(RtcClock::MIN_TIMESTAMP - 1).has_value());
    CHECK_FALSE(RtcClock::toRtcTime(RtcClock::MAX_TIMESTAMP + 1).has_value());
}

TEST(RtcClock, ConversionsAreInverse)
{
    for(npos::dev::Timestamp t = RtcClock::MIN_TIMESTAMP; t < RtcClock::MAX_TIMESTAMP; t += 86399 * 37)
    {
        auto result = RtcClock::toRtcTime(t);

        CHECK_TRUE(result.has_value());
        CHECK_EQUAL(t, RtcClock::fromRtcTime(result->first, result->second));
    }
}

TEST(RtcClock, AcceptsClockAndOpenMessages)
{
    CHECK_TRUE(clock.accepts(npos::dev::msg::Type::CLOCK));
    CHECK_TRUE(clock.accepts(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(clock.accepts(npos::dev::msg::Type::LED));
}

TEST(RtcClock, OpenInitializesRtcOnce)
{
    open();
    open();

    CHECK_EQUAL(1, rtc.initCount);
}

TEST(RtcClock, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    clock.handle(request);

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(0, rtc.initCount);
}

TEST(RtcClock, OpenReportsInitFailure)
{
    rtc.failInit = true;
    npos::dev::msg::Open request("clock");

    clock.handle(request);

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, statusOfResponse());
}

TEST(RtcClock, GetTimeBeforeOpenFails)
{
    Clock request({ 3, 0 }, Clock::GET_TIME);

    clock.handle(request);

    CHECK_EQUAL(Clock::NOT_INITIALIZED, statusOfResponse());
}

TEST(RtcClock, GetTimeReturnsRtcTimestamp)
{
    open();
    rtc.time = { 12, 50, 24 };
    rtc.date = { 29, 9, 26 };
    Clock request({ 3, 0 }, Clock::GET_TIME);

    clock.handle(request);

    CHECK_EQUAL(Clock::SUCCESS, statusOfResponse());
    CHECK_EQUAL(1790686224, api.responded[0].output.attributes.value);
}

TEST(RtcClock, GetTimeReportsDeviceFailures)
{
    open();
    Clock request({ 3, 0 }, Clock::GET_TIME);

    rtc.failGetTime = true;
    clock.handle(request);
    rtc.failGetTime = false;
    rtc.failGetDate = true;
    clock.handle(request);

    CHECK_EQUAL(Clock::DEVICE_FAILURE, api.responded[0].output.status);
    CHECK_EQUAL(Clock::DEVICE_FAILURE, api.responded[1].output.status);
}

TEST(RtcClock, SetTimeBeforeOpenFails)
{
    Clock request({ 3, 0 }, Clock::SET_TIME, 1790686224);

    clock.handle(request);

    CHECK_EQUAL(Clock::NOT_INITIALIZED, statusOfResponse());
}

TEST(RtcClock, SetTimeProgramsRtcAndEchoesTimestamp)
{
    open();
    Clock request({ 3, 0 }, Clock::SET_TIME, 1790686224);

    clock.handle(request);

    CHECK_EQUAL(Clock::SUCCESS, statusOfResponse());
    CHECK_EQUAL(1790686224, api.responded[0].output.attributes.value);
    CHECK_EQUAL(12, rtc.time.hours);
    CHECK_EQUAL(9, rtc.date.month);
}

TEST(RtcClock, SetTimeRejectsInvalidTimestamp)
{
    open();
    Clock request({ 3, 0 }, Clock::SET_TIME, 5);

    clock.handle(request);

    CHECK_EQUAL(Clock::INVALID_TIMESTAMP, statusOfResponse());
}

TEST(RtcClock, SetTimeReportsDeviceFailures)
{
    open();
    Clock request({ 3, 0 }, Clock::SET_TIME, 1790686224);

    rtc.failSetTime = true;
    clock.handle(request);
    rtc.failSetTime = false;
    rtc.failSetDate = true;
    clock.handle(request);

    CHECK_EQUAL(Clock::DEVICE_FAILURE, api.responded[0].output.status);
    CHECK_EQUAL(Clock::DEVICE_FAILURE, api.responded[1].output.status);
}
