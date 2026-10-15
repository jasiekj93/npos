/**
 * @file HashProcessor.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeHash.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/HashProcessor.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Hash;
using npos::dev::hal::Interrupt;

TEST_GROUP(HashProcessor)
{
    mock::RecordingApi api;
    mock::FakeHash hash;
    npos::dev::InterruptService::InterruptQueue<4> queue;
    npos::dev::service::HashProcessor processor{api, queue, hash, "hash"};
    uint8_t input[16] = {};
    uint8_t digest[npos::dev::hal::Hash::DIGEST_SIZE];

    void setup() override { processor.setId(2); }

    void open()
    {
        npos::dev::msg::Open request("hash");
        processor.handleRequest(request);
        api.responded.clear();
    }

    void interrupt(npos::dev::hal::Interrupt::Code code, Interrupt::Type type = Interrupt::HASH)
    {
        hash.fire(type, code);
        processor.processInterrupt();
    }

    Hash accumulate() { return Hash({ 3, 2 }, Hash::ACCUMULATE, etl::span<const uint8_t>(input, sizeof(input))); }
    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(HashProcessor, UnsupportedOperationIsAnsweredWithNotSupported)
{
    open();
    auto request = accumulate();
    request.input.readWrite.mode = 99;

    processor.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, statusOfResponse());
}

TEST(HashProcessor, AccumulateThatCannotStartIsAnsweredAndDoesNotBlockTheDevice)
{
    open();
    hash.failAccumulate = true;
    auto failed = accumulate();

    processor.handleRequest(failed);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(Hash::DEVICE_FAILURE, statusOfResponse());
    CHECK_EQUAL(0, failed.referenceCount);

    api.responded.clear();
    hash.failAccumulate = false;
    auto next = accumulate();
    processor.handleRequest(next);

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(1, next.referenceCount);
}

TEST(HashProcessor, AccumulateEndThatCannotStartIsAnswered)
{
    open();
    hash.failAccumulate = true;
    Hash request({ 3, 2 }, Hash::ACCUMULATE_END, etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, sizeof(digest)));

    processor.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(Hash::DEVICE_FAILURE, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(HashProcessor, AcceptsHashAndOpenMessages)
{
    CHECK_TRUE(processor.acceptsRequest(npos::dev::msg::Type::HASH));
    CHECK_TRUE(processor.acceptsRequest(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(processor.acceptsRequest(npos::dev::msg::Type::LED));
}

TEST(HashProcessor, OpenInitializesHashOnceAndAnswersWithOid)
{
    npos::dev::msg::Open request("hash");

    processor.handleRequest(request);
    processor.handleRequest(request);

    CHECK_EQUAL(1, hash.initCount);
    CHECK_EQUAL(2, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
    CHECK_EQUAL(2, api.responded[0].output.open.opened.serviceId);
}

TEST(HashProcessor, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    processor.handleRequest(request);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(HashProcessor, OpenReportsInitFailure)
{
    hash.failInit = true;
    npos::dev::msg::Open request("hash");

    processor.handleRequest(request);

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, statusOfResponse());
}

TEST(HashProcessor, RequestBeforeOpenIsRejected)
{
    auto request = accumulate();

    processor.handleRequest(request);

    CHECK_EQUAL(Hash::NOT_INITIALIZED, statusOfResponse());
}

TEST(HashProcessor, AccumulateStartsHardwareAndHoldsRequestUntilInterrupt)
{
    open();
    auto request = accumulate();

    processor.handleRequest(request);

    CHECK_EQUAL(sizeof(input), hash.accumulated);
    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(1, request.referenceCount);
}

TEST(HashProcessor, InputCompleteAnswersSuccessAndReleasesRequest)
{
    open();
    auto request = accumulate();
    processor.handleRequest(request);

    interrupt(npos::dev::hal::Hash::INPUT_COMPLETE);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(Hash::SUCCESS, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(HashProcessor, SecondRequestWhileBusyIsRejected)
{
    open();
    auto first = accumulate();
    auto second = accumulate();
    processor.handleRequest(first);

    processor.handleRequest(second);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(Hash::BUSY, statusOfResponse());
}

TEST(HashProcessor, AccumulateEndWithWrongOutputSizeIsRejected)
{
    open();
    Hash request({ 3, 2 }, Hash::ACCUMULATE_END, etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, 4));

    processor.handleRequest(request);

    CHECK_EQUAL(Hash::INVALID_OUTPUT_SIZE, statusOfResponse());
}

TEST(HashProcessor, AccumulateEndFinishesWithDigestInterrupt)
{
    open();
    Hash request({ 3, 2 }, Hash::ACCUMULATE_END, etl::span<const uint8_t>(input, sizeof(input)), etl::span<uint8_t>(digest, sizeof(digest)));

    processor.handleRequest(request);
    interrupt(npos::dev::hal::Hash::DIGEST_COMPLETE);

    CHECK_EQUAL(sizeof(digest), hash.digestSize);
    CHECK_EQUAL(Hash::SUCCESS, statusOfResponse());
}

TEST(HashProcessor, ErrorInterruptReportsDeviceFailure)
{
    open();
    auto request = accumulate();
    processor.handleRequest(request);

    interrupt(npos::dev::hal::Hash::ERROR);

    CHECK_EQUAL(Hash::DEVICE_FAILURE, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(HashProcessor, InterruptWithoutRequestIsIgnored)
{
    open();

    interrupt(npos::dev::hal::Hash::INPUT_COMPLETE);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(HashProcessor, InterruptFromOtherPeripheralIsIgnored)
{
    open();
    auto request = accumulate();
    processor.handleRequest(request);

    interrupt(npos::dev::hal::Hash::INPUT_COMPLETE, Interrupt::UART);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(HashProcessor, UnknownEventKeepsRequestPending)
{
    open();
    auto request = accumulate();
    processor.handleRequest(request);

    interrupt(0x7F);

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(1, request.referenceCount);
}

TEST(HashProcessor, CanServeNextRequestAfterCompletion)
{
    open();
    auto first = accumulate();
    processor.handleRequest(first);
    interrupt(npos::dev::hal::Hash::INPUT_COMPLETE);
    api.responded.clear();
    auto second = accumulate();

    processor.handleRequest(second);
    interrupt(npos::dev::hal::Hash::INPUT_COMPLETE);

    CHECK_EQUAL(Hash::SUCCESS, statusOfResponse());
}
