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

TEST(Message, DefaultsToNullSenderRecipientRequestAndNoReferences)
{
    npos::ipc::Message message;

    CHECK_EQUAL(npos::ipc::NULL_PORT, message.sender.portId);
    CHECK_EQUAL(npos::ipc::NULL_SERVICE, message.sender.serviceId);
    CHECK_EQUAL(npos::ipc::NULL_PORT, message.recipient.portId);
    CHECK_EQUAL(npos::ipc::NULL_SERVICE, message.recipient.serviceId);
    CHECK_EQUAL(0, message.referenceCount);
}

TEST(Message, OutputStatusDefaultsToNotSupported)
{
    npos::ipc::Message message;

    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, message.output.status);
}

TEST(Message, StoreMessageTakesAReferenceAndReturnsTheDerivedType)
{
    struct Derived : public npos::ipc::Message {};

    Derived message;
    npos::ipc::Message& base = message;

    Derived* stored = npos::ipc::storeMessage<Derived>(base);

    POINTERS_EQUAL(&message, stored);
    CHECK_EQUAL(1, message.referenceCount);

    npos::ipc::storeMessage<Derived>(base);

    CHECK_EQUAL(2, message.referenceCount);
}

TEST(Message, ReleaseMessageDropsTheReferenceAndClearsThePointer)
{
    struct Derived : public npos::ipc::Message {};

    Derived message;
    Derived* stored = npos::ipc::storeMessage<Derived>(message);

    npos::ipc::releaseMessage(&stored);

    CHECK_EQUAL(0, message.referenceCount);
    CHECK_TRUE(stored == nullptr);
}

TEST(Message, ReleaseMessageKeepsOtherReferences)
{
    struct Derived : public npos::ipc::Message {};

    Derived message;
    message.referenceCount = 2;
    Derived* stored = &message;

    npos::ipc::releaseMessage(&stored);

    CHECK_EQUAL(1, message.referenceCount);
}

TEST(Message, ReleaseMessageNeverUnderflows)
{
    struct Derived : public npos::ipc::Message {};

    Derived message;
    Derived* stored = &message;

    npos::ipc::releaseMessage(&stored);

    CHECK_EQUAL(0, message.referenceCount);
    CHECK_TRUE(stored == nullptr);
}

TEST(Message, ReleaseMessageIgnoresNullPointers)
{
    struct Derived : public npos::ipc::Message {};

    Derived* none = nullptr;

    npos::ipc::releaseMessage(&none);
    npos::ipc::releaseMessage<Derived>(nullptr);

    CHECK_TRUE(none == nullptr);
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
