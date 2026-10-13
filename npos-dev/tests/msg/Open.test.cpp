/**
 * @file Open.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/Open.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DevOpen)
{
};

TEST(DevOpen, StoresDeviceName)
{
    npos::dev::msg::Open request("uart0");

    CHECK_EQUAL(npos::dev::msg::Type::OPEN, request.type);
    CHECK_TRUE(request.getName() == "uart0");
}

TEST(DevOpen, IsBroadcastUntilAnswered)
{
    npos::dev::msg::Open request("x");

    CHECK_EQUAL(npos::ipc::NULL_PORT, request.recipient.portId);
}

TEST(DevOpen, ResponseCarriesStatusAndOpenedOid)
{
    npos::dev::msg::Open request("x");

    request.setStatus(npos::dev::msg::Open::INIT_FAILURE).setOpened({ 4, 2 });

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, request.getStatus());
    CHECK_EQUAL(4, request.getOpened().portId);
    CHECK_EQUAL(2, request.getOpened().serviceId);
}
