/**
 * @file Message.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/ipc/Message.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(Message)
{
};

TEST(Message, DefaultsToNullSenderRecipientAndNoReferences)
{
    npos::ipc::Message message;

    CHECK_EQUAL(npos::ipc::NULL_PORT, message.sender);
    CHECK_EQUAL(npos::ipc::NULL_PORT, message.recipient.portId);
    CHECK_EQUAL(npos::ipc::NULL_SERVICE, message.recipient.serviceId);
    CHECK_EQUAL(0, message.referenceCount);
}

TEST(Message, InputRawOverlapsTypedFields)
{
    npos::ipc::Message message;
    message.input.readWrite = {};
    message.input.readWrite.mode = 5;

    CHECK_EQUAL(npos::ipc::Message::RAW_SIZE, sizeof(message.input.raw));
    CHECK_TRUE(sizeof(message.input.readWrite) <= sizeof(message.input.raw));
    CHECK_TRUE(sizeof(message.input.link) <= sizeof(message.input.raw));
}

TEST(Message, OutputRawOverlapsTypedFields)
{
    npos::ipc::Message message;

    CHECK_EQUAL(npos::ipc::Message::RAW_SIZE, sizeof(message.output.raw));
    CHECK_TRUE(sizeof(message.output.open) <= sizeof(message.output.raw));
    CHECK_TRUE(sizeof(message.output.readWrite) <= sizeof(message.output.raw));
}

TEST(Message, TypeRangesAreAscendingAndDistinct)
{
    CHECK_TRUE(npos::ipc::OS_ERROR_MESSAGE_TYPE < npos::ipc::OS_SYSLOG_MESSAGE_TYPE);
    CHECK_TRUE(npos::ipc::OS_SYSLOG_MESSAGE_TYPE < npos::ipc::DEVICE_MESSAGE_TYPE);
    CHECK_TRUE(npos::ipc::DEVICE_MESSAGE_TYPE < npos::ipc::DRIVER_MESSAGE_TYPE);
    CHECK_TRUE(npos::ipc::DRIVER_MESSAGE_TYPE < npos::ipc::FILESYSTEM_MESSAGE_TYPE);
    CHECK_TRUE(npos::ipc::FILESYSTEM_MESSAGE_TYPE < npos::ipc::CLI_MESSAGE_TYPE);
    CHECK_TRUE(npos::ipc::CLI_MESSAGE_TYPE < npos::ipc::USER_MESSAGE_TYPE);
}
