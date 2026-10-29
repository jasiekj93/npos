/**
 * @file InterruptService.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeHash.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/InterruptService.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Recorder : public npos::dev::InterruptService
    {
    public:
        using npos::dev::InterruptService::InterruptService;

        void handleRequest(npos::ipc::Message&) override {}
        void handleResponse(npos::ipc::Message&) override {}
        bool acceptsRequest(npos::ipc::Message::Type) const override { return false; }

        void onInterrupt(const npos::dev::hal::Interrupt& interrupt) override { codes.push_back(interrupt.code); }

        etl::vector<int, 8> codes;
    };
}

TEST_GROUP(InterruptService)
{
    mock::RecordingApi api;
    mock::FakeHash device;
    npos::dev::InterruptService::InterruptQueue<2> queue;
    Recorder service{api, queue, device};
};

TEST(InterruptService, RegistersItselfAsDeviceHandler)
{
    CHECK_TRUE(device.source.handler != nullptr);
}

TEST(InterruptService, NoInterruptIsPendingInitially)
{
    CHECK_FALSE(service.isInterruptPending());
}

TEST(InterruptService, InterruptFromDeviceBecomesPending)
{
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 1);

    CHECK_TRUE(service.isInterruptPending());
}

TEST(InterruptService, PendingCheckGuardsDeviceInterrupts)
{
    service.isInterruptPending();

    CHECK_EQUAL(1, device.source.disableCount);
    CHECK_EQUAL(1, device.source.enableCount);
}

TEST(InterruptService, ProcessInterruptDeliversInOrder)
{
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 1);
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 2);

    service.processInterrupt();
    service.processInterrupt();

    CHECK_EQUAL(2, service.codes.size());
    CHECK_EQUAL(1, service.codes[0]);
    CHECK_EQUAL(2, service.codes[1]);
    CHECK_FALSE(service.isInterruptPending());
}

TEST(InterruptService, ProcessInterruptWithoutPendingDoesNothing)
{
    service.processInterrupt();

    CHECK_EQUAL(0, service.codes.size());
    CHECK_EQUAL(device.source.disableCount, device.source.enableCount);
}

TEST(InterruptService, InterruptsAreDroppedWhenQueueIsFull)
{
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 1);
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 2);
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 3);

    service.processInterrupt();
    service.processInterrupt();
    service.processInterrupt();

    CHECK_EQUAL(2, service.codes.size());
}
