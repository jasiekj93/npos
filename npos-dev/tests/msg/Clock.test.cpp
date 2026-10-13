/**
 * @file Clock.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Clock.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Clock)
{
};

TEST(Clock, GetTimeRequestIsAddressedToDevice)
{
    npos::dev::msg::Clock request({ 1, 2 }, npos::dev::msg::Clock::GET_TIME);

    CHECK_EQUAL(npos::dev::msg::Type::CLOCK, request.type);
    CHECK_EQUAL(2, request.recipient.serviceId);
    CHECK_EQUAL(npos::dev::msg::Clock::GET_TIME, request.getOperation());
    CHECK_EQUAL(0, request.getInputTimestamp());
}

TEST(Clock, SetTimeRequestCarriesTimestamp)
{
    npos::dev::msg::Clock request({ 1, 2 }, npos::dev::msg::Clock::SET_TIME, 1790686224);

    CHECK_EQUAL(npos::dev::msg::Clock::SET_TIME, request.getOperation());
    CHECK_EQUAL(1790686224, request.getInputTimestamp());
}

TEST(Clock, ResponseCarriesStatusAndTimestamp)
{
    npos::dev::msg::Clock request({ 1, 2 }, npos::dev::msg::Clock::GET_TIME);

    request.setStatus(npos::dev::msg::Clock::SUCCESS).setOutputTimestamp(42);

    CHECK_EQUAL(npos::dev::msg::Clock::SUCCESS, request.getStatus());
    CHECK_EQUAL(42, request.getOutputTimestamp());
}
