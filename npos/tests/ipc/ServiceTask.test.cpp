/**
 * @file ServiceTask.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBus.hpp"
#include "mock/FakeService.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/ipc/ServiceTask.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(ServiceTask)
{
    mock::FakeBus bus;
    mock::RecordingApi api;
    npos::ipc::ServiceTask::ServiceList<2> serviceList;
    npos::ipc::ServiceTask::MessageQueue<2> queue;
    npos::ipc::ServiceTask task{4, serviceList, queue, bus};
    mock::FakeService first{task, 0x1000};
    mock::FakeService second{task, 0x2000};

    void setup() override
    {
        task.setId(6);
    }
};

TEST(ServiceTask, KeepsPriority)
{
    CHECK_EQUAL(4, task.getPriority());
}

TEST(ServiceTask, SubscribeAssignsConsecutiveServiceIds)
{
    CHECK_TRUE(task.subscribe(first));
    CHECK_TRUE(task.subscribe(second));

    CHECK_EQUAL(0, first.getOid().serviceId);
    CHECK_EQUAL(1, second.getOid().serviceId);
    CHECK_EQUAL(6, first.getOid().portId);
}

TEST(ServiceTask, SubscribeFailsWhenFull)
{
    mock::FakeService third{task};
    task.subscribe(first);
    task.subscribe(second);

    CHECK_FALSE(task.subscribe(third));
}

TEST(ServiceTask, AcceptsWhatAnyServiceAccepts)
{
    task.subscribe(first);
    task.subscribe(second);

    CHECK_TRUE(task.accepts(0x1000));
    CHECK_TRUE(task.accepts(0x2000));
    CHECK_FALSE(task.accepts(0x3000));
}

TEST(ServiceTask, InitializeInitializesAllServices)
{
    task.subscribe(first);
    task.subscribe(second);

    task.initialize();

    CHECK_EQUAL(1, first.initialized);
    CHECK_EQUAL(1, second.initialized);
}

TEST(ServiceTask, IsReadyOnlyWithQueuedMessages)
{
    npos::ipc::Message message;

    CHECK_FALSE(task.isReady());

    task.receive(message);

    CHECK_TRUE(task.isReady());
}

TEST(ServiceTask, ReceiveDropsMessagesWhenQueueIsFull)
{
    npos::ipc::Message a, b, c;
    a.recipient = b.recipient = c.recipient = { 6, npos::ipc::NULL_SERVICE };
    a.type = b.type = c.type = 0x1000;
    task.subscribe(first);

    task.receive(a);
    task.receive(b);
    task.receive(c);
    task.process();
    task.process();

    CHECK_EQUAL(2, first.handled.size());
    CHECK_FALSE(task.isReady());
}

TEST(ServiceTask, ProcessDispatchesToAddressedService)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x2000;
    message.recipient = { 6, 1 };

    task.receive(message);
    task.process();

    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(1, second.handled.size());
}

TEST(ServiceTask, ProcessDispatchesResponseToSenderService)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.isResponse = true;
    message.sender = { 6, 1 };
    message.recipient = { 7, 0 };

    task.receive(message);
    task.process();

    CHECK_EQUAL(0, first.handledResponses.size());
    CHECK_EQUAL(1, second.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(0, second.handled.size());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(ServiceTask, ProcessIgnoresResponseFromOutOfRangeSenderService)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.isResponse = true;
    message.sender = { 6, 9 };

    task.receive(message);
    task.process();

    CHECK_EQUAL(0, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(ServiceTask, ProcessIgnoresResponseWithoutSenderService)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.isResponse = true;

    task.receive(message);
    task.process();

    CHECK_EQUAL(0, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
}

TEST(ServiceTask, ProcessIgnoresOutOfRangeServiceId)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.recipient = { 6, 9 };

    task.receive(message);
    task.process();

    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(ServiceTask, ProcessBroadcastsToAcceptingServicesOnly)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x1000;

    task.receive(message);
    task.process();

    CHECK_EQUAL(1, first.handled.size());
    CHECK_EQUAL(0, second.handled.size());
}

TEST(ServiceTask, ProcessReleasesMessageAndPopsQueue)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;

    task.receive(message);
    task.process();

    CHECK_EQUAL(1, bus.released.size());
    POINTERS_EQUAL(&message, bus.released[0]);
    CHECK_FALSE(task.isReady());
}

TEST(ServiceTask, ProcessWithoutMessagesDoesNothing)
{
    task.process();

    CHECK_EQUAL(0, bus.released.size());
}

TEST(ServiceTask, SendToOtherPortGoesThroughBusWithSenderSet)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.recipient = { 7, 0 };

    task.send(second, message);

    CHECK_EQUAL(1, bus.sent.size());
    POINTERS_EQUAL(&message, bus.sent[0]);
    CHECK_EQUAL(6, message.sender.portId);
    CHECK_EQUAL(1, message.sender.serviceId);
    CHECK_FALSE(message.isResponse);
}

TEST(ServiceTask, SendMarksMessageAsRequest)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.recipient = { 7, 0 };
    message.isResponse = true;

    task.send(first, message);

    CHECK_FALSE(message.isResponse);
    CHECK_EQUAL(0, bus.responded.size());
}

TEST(ServiceTask, SendWithinTheSameTaskIsDispatchedWithoutBus)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.recipient = { 6, 0 };

    task.send(first, message);

    CHECK_EQUAL(0, bus.sent.size());
    CHECK_EQUAL(1, first.handled.size());
    CHECK_EQUAL(0, first.handledResponses.size());
}

TEST(ServiceTask, BroadcastSendGoesThroughBus)
{
    task.subscribe(first);
    npos::ipc::Message message;

    task.send(first, message);

    CHECK_EQUAL(1, bus.sent.size());
}

TEST(ServiceTask, RespondToOwnSenderIsDispatchedLocally)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.sender = { 6, 0 };
    message.recipient = { 7, 0 };

    task.respond(message);

    CHECK_TRUE(message.isResponse);
    CHECK_EQUAL(0, bus.responded.size());
    CHECK_EQUAL(0, bus.sent.size());
    CHECK_EQUAL(1, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
}

TEST(ServiceTask, RespondToOtherSenderGoesThroughBus)
{
    npos::ipc::Message message;
    message.sender = { 2, 0 };

    task.respond(message);

    CHECK_TRUE(message.isResponse);
    CHECK_EQUAL(1, bus.responded.size());
    POINTERS_EQUAL(&message, bus.responded[0]);
}

TEST(ServiceTask, PortIdMatchesAssignedId)
{
    CHECK_EQUAL(6, task.getPortId());
}
