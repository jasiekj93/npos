/**
 * @file InterruptServiceTask.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBus.hpp"
#include "mock/FakeHash.hpp"

#include <libnpos/dev/InterruptServiceTask.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Recorder : public npos::dev::InterruptService
    {
    public:
        using npos::dev::InterruptService::InterruptService;

        void handleRequest(npos::ipc::Message&) override { handled++; }
        void handleResponse(npos::ipc::Message&) override { responses++; }
        bool acceptsRequest(npos::ipc::Message::Type type) const override { return type == 0x1000; }
        void onInterrupt(const npos::dev::hal::Interrupt&) override { interrupts++; }

        int handled = 0;
        int responses = 0;
        int interrupts = 0;
    };
}

TEST_GROUP(InterruptServiceTask)
{
    mock::FakeBus bus;
    mock::FakeHash device;
    npos::dev::InterruptService::InterruptQueue<4> interruptQueue;
    npos::dev::InterruptServiceTask::InterruptServiceList<2> services;
    npos::dev::InterruptServiceTask::RequestQueue<4> requests;
    npos::dev::InterruptServiceTask::ResponseQueue<4> responses;
    npos::dev::InterruptServiceTask task{5, services, requests, responses, bus};
    Recorder service{task, interruptQueue, device};

    void setup() override
    {
        task.setId(1);
        task.subscribe(service);
    }
};

TEST(InterruptServiceTask, IsNotReadyWithoutWork)
{
    CHECK_FALSE(task.isReady());
}

TEST(InterruptServiceTask, IsReadyWhenServiceHasPendingInterrupt)
{
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 0);

    CHECK_TRUE(task.isReady());
}

TEST(InterruptServiceTask, IsReadyWhenMessageIsQueued)
{
    npos::ipc::Message message;
    task.receiveRequest(message);

    CHECK_TRUE(task.isReady());
}

TEST(InterruptServiceTask, ProcessHandlesInterruptBeforeMessages)
{
    npos::ipc::Message message;
    message.type = 0x1000;
    task.receiveRequest(message);
    device.fire(npos::dev::hal::Interrupt::HASH_INT, 0);

    task.process();

    CHECK_EQUAL(1, service.interrupts);
    CHECK_EQUAL(0, service.handled);
    CHECK_TRUE(task.isReady());

    task.process();

    CHECK_EQUAL(1, service.handled);
}

TEST(InterruptServiceTask, AcceptsWhatItsServicesAccept)
{
    CHECK_TRUE(task.accepts(0x1000));
    CHECK_FALSE(task.accepts(0x2000));
}
