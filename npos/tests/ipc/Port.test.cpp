/**
 * @file Port.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakePort.hpp"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Port)
{
    mock::FakePort port;
};

TEST(Port, HasNullIdUntilSet)
{
    CHECK_EQUAL(npos::ipc::NULL_PORT, port.getId());
}

TEST(Port, KeepsAssignedId)
{
    port.setId(12);

    CHECK_EQUAL(12, port.getId());
}

TEST(Port, ReceivesAndAcceptsThroughInterface)
{
    npos::ipc::Port& base = port;
    npos::ipc::Message message;
    message.type = 0x1000;

    CHECK_TRUE(base.accepts(0x1000));
    CHECK_FALSE(base.accepts(0x2000));

    base.receive(message);

    CHECK_EQUAL(1, port.received.size());
    POINTERS_EQUAL(&message, port.received[0]);
}
