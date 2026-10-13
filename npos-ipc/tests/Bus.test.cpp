/**
 * @file Bus.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBus.hpp"

#include <CppUTest/TestHarness.h>

TEST_GROUP(Bus)
{
    mock::FakeBus fake;
    npos::ipc::Bus& bus = fake;
};

TEST(Bus, SendRespondAndReleaseAreDispatchedThroughInterface)
{
    npos::ipc::Message message;

    bus.send(message);
    bus.respond(message);
    bus.release(&message);

    CHECK_EQUAL(1, fake.sent.size());
    CHECK_EQUAL(1, fake.responded.size());
    CHECK_EQUAL(1, fake.released.size());
}
