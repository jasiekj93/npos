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
    npos::ipc::Message request;
    npos::ipc::Message response;
    request.type = 0x1000;

    CHECK_TRUE(base.accepts(0x1000));
    CHECK_FALSE(base.accepts(0x2000));

    base.receiveRequest(request);
    base.receiveResponse(response);

    CHECK_EQUAL(1, port.received.size());
    POINTERS_EQUAL(&request, port.received[0]);
    CHECK_EQUAL(1, port.receivedResponses.size());
    POINTERS_EQUAL(&response, port.receivedResponses[0]);
}

TEST(Port, UnknownRequestIsPassedThroughInterface)
{
    npos::ipc::Port& base = port;
    npos::ipc::Message message;

    base.handleUnknownRequest(message);

    CHECK_EQUAL(1, port.unknown.size());
    POINTERS_EQUAL(&message, port.unknown[0]);
}
