/**
 * @file Led.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Led.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Led)
{
};

TEST(Led, StoresIdAndOperation)
{
    npos::dev::msg::Led request({ 1, 2 }, 3, npos::dev::msg::Led::TOGGLE);

    CHECK_EQUAL(npos::dev::msg::Type::LED, request.type);
    CHECK_EQUAL(3, request.getId());
    CHECK_EQUAL(npos::dev::msg::Led::TOGGLE, request.getOperation());
    CHECK_EQUAL(2, request.recipient.serviceId);
}

TEST(Led, ResponseCarriesStatus)
{
    npos::dev::msg::Led request({ 1, 2 }, 0, npos::dev::msg::Led::ON);

    request.setStatus(npos::dev::msg::Led::LED_NOT_FOUND);

    CHECK_EQUAL(npos::dev::msg::Led::LED_NOT_FOUND, request.getStatus());
}
