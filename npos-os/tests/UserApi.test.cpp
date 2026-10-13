/**
 * @file UserApi.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/RecordingApi.hpp"

#include <libnpos/os/UserApi.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(UserApi)
{
    npos::os::UserApi userApi;
    mock::RecordingApi target;
};

TEST(UserApi, IsNotBoundInitially)
{
    CHECK_FALSE(userApi.isBound());
}

TEST(UserApi, UnboundApiHasNullPortAndDropsMessages)
{
    npos::ipc::Message message;

    userApi.send(message);
    userApi.respond(message);

    CHECK_EQUAL(npos::ipc::NULL_PORT, userApi.getPortId());
    CHECK_EQUAL(0, target.sent.size());
}

TEST(UserApi, BoundApiForwardsToTarget)
{
    npos::ipc::Message message;
    message.type = 9;

    userApi.bind(target);
    userApi.send(message);
    userApi.respond(message);

    CHECK_TRUE(userApi.isBound());
    CHECK_EQUAL(3, userApi.getPortId());
    CHECK_EQUAL(1, target.sent.size());
    CHECK_EQUAL(1, target.responded.size());
    CHECK_EQUAL(9, target.sent[0].type);
}
