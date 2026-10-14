/**
 * @file Timer.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Timer.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Timer)
{
};

TEST(Timer, StoresModeIntervalAndTarget)
{
    npos::dev::msg::Timer request({ 2, 3 }, npos::dev::msg::Timer::REPEATING, 1000);

    CHECK_EQUAL(npos::dev::msg::Type::TIMER, request.type);
    CHECK_EQUAL(npos::dev::msg::Timer::REPEATING, request.getMode());
    CHECK_EQUAL(1000, request.getInterval());
    CHECK_EQUAL(2, request.getTarget().portId);
    CHECK_EQUAL(3, request.getTarget().serviceId);
}

TEST(Timer, ResponseCarriesStatusAndEventTick)
{
    npos::dev::msg::Timer request({ 2, 3 }, npos::dev::msg::Timer::SINGLE_SHOT, 10);

    request.setStatus(npos::dev::msg::Timer::NO_MEMORY).setEventTick(55);

    CHECK_EQUAL(npos::dev::msg::Timer::NO_MEMORY, request.getStatus());
    CHECK_EQUAL(55, request.getEventTick());
}
