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
    npos::ipc::ServiceTask::RequestQueue<2> requests;
    npos::ipc::ServiceTask::ResponseQueue<2> responses;
    npos::ipc::ServiceTask task{4, serviceList, requests, responses, bus};
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

    task.receiveRequest(message);

    CHECK_TRUE(task.isReady());
}

TEST(ServiceTask, IsReadyWithQueuedResponse)
{
    npos::ipc::Message message;

    task.receiveResponse(message);

    CHECK_TRUE(task.isReady());
}

TEST(ServiceTask, ReceiveReleasesRequestsWhenRequestQueueIsFull)
{
    npos::ipc::Message a, b, c;
    a.recipient = b.recipient = c.recipient = { 6, npos::ipc::NULL_SERVICE };
    a.type = b.type = c.type = 0x1000;
    task.subscribe(first);

    task.receiveRequest(a);
    task.receiveRequest(b);
    task.receiveRequest(c);

    CHECK_EQUAL(1, bus.released.size());
    POINTERS_EQUAL(&c, bus.released[0]);

    task.process();
    task.process();

    CHECK_EQUAL(2, first.handled.size());
    CHECK_FALSE(task.isReady());
}

TEST(ServiceTask, ReceiveReleasesResponsesWhenResponseQueueIsFull)
{
    npos::ipc::Message a, b, c;
    a.sender = b.sender = c.sender = { 6, 0 };
    task.subscribe(first);

    task.receiveResponse(a);
    task.receiveResponse(b);
    task.receiveResponse(c);

    CHECK_EQUAL(1, bus.released.size());
    POINTERS_EQUAL(&c, bus.released[0]);
}

TEST(ServiceTask, FullRequestQueueDoesNotBlockResponses)
{
    npos::ipc::Message a, b, response;
    a.type = b.type = 0x1000;
    response.sender = { 6, 0 };
    task.subscribe(first);
    task.receiveRequest(a);
    task.receiveRequest(b);

    task.receiveResponse(response);
    task.process();

    CHECK_EQUAL(1, bus.released.size());
    CHECK_EQUAL(1, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
}

TEST(ServiceTask, ProcessHandlesResponsesBeforeRequests)
{
    npos::ipc::Message request, response;
    request.type = 0x1000;
    response.sender = { 6, 0 };
    task.subscribe(first);
    task.receiveRequest(request);
    task.receiveResponse(response);

    task.process();

    CHECK_EQUAL(1, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
    CHECK_TRUE(task.isReady());

    task.process();

    CHECK_EQUAL(1, first.handled.size());
    CHECK_FALSE(task.isReady());
}

TEST(ServiceTask, ProcessDispatchesToAddressedService)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x2000;
    message.recipient = { 6, 1 };

    task.receiveRequest(message);
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
    message.sender = { 6, 1 };
    message.recipient = { 7, 0 };

    task.receiveResponse(message);
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
    message.sender = { 6, 9 };

    task.receiveResponse(message);
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

    task.receiveResponse(message);
    task.process();

    CHECK_EQUAL(0, first.handledResponses.size());
    CHECK_EQUAL(0, first.handled.size());
}

TEST(ServiceTask, ProcessRejectsOutOfRangeServiceId)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;
    message.sender = { 2, 0 };
    message.recipient = { 6, 9 };

    task.receiveRequest(message);
    task.process();

    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(0, first.handledUnknown.size());
    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_SERVICE_NOT_FOUND, message.output.status);
    CHECK_EQUAL(1, bus.released.size());
}

TEST(ServiceTask, ProcessRejectsRequestTheAddressedServiceDoesNotAccept)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x2000;
    message.sender = { 2, 0 };
    message.recipient = { 6, 0 };
    message.output.status = 0;

    task.receiveRequest(message);
    task.process();

    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(0, second.handled.size());
    CHECK_EQUAL(1, first.handledUnknown.size());
    CHECK_EQUAL(0, second.handledUnknown.size());
    CHECK_EQUAL(1, bus.responded.size());
    POINTERS_EQUAL(&message, bus.responded[0]);
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, message.output.status);
}

TEST(ServiceTask, ProcessRejectsRequestForThePortThatNoServiceAccepts)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x3000;
    message.sender = { 2, 0 };
    message.recipient = { 6, npos::ipc::NULL_SERVICE };

    task.receiveRequest(message);
    task.process();

    CHECK_EQUAL(0, first.handled.size());
    CHECK_EQUAL(0, first.handledUnknown.size());
    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_SERVICE_NOT_FOUND, message.output.status);
}

TEST(ServiceTask, ProcessDoesNotAnswerBroadcastNobodyAccepts)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x3000;
    message.sender = { 2, 0 };

    task.receiveRequest(message);
    task.process();

    CHECK_EQUAL(0, bus.responded.size());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(ServiceTask, UnknownRequestInTheSameTaskIsAnsweredLocally)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x2000;
    message.recipient = { 6, 0 };

    task.send(second, message);

    CHECK_EQUAL(0, bus.responded.size());
    CHECK_EQUAL(1, first.handledUnknown.size());
    CHECK_EQUAL(1, second.handledResponses.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, message.output.status);
}

TEST(ServiceTask, PortLevelUnknownRequestIsPassedToTheAddressedService)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.recipient = { 6, 1 };

    task.handleUnknownRequest(message);

    CHECK_EQUAL(0, first.handledUnknown.size());
    CHECK_EQUAL(1, second.handledUnknown.size());
    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, message.output.status);
}

TEST(ServiceTask, PortLevelUnknownRequestWithoutServiceIsAnsweredWithServiceNotFound)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.recipient = { 6, npos::ipc::NULL_SERVICE };

    task.handleUnknownRequest(message);

    CHECK_EQUAL(0, first.handledUnknown.size());
    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_SERVICE_NOT_FOUND, message.output.status);

    message.recipient.serviceId = 9;
    message.output.status = 0;
    task.handleUnknownRequest(message);

    CHECK_EQUAL(2, bus.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_SERVICE_NOT_FOUND, message.output.status);
}

TEST(ServiceTask, ProcessBroadcastsToAcceptingServicesOnly)
{
    task.subscribe(first);
    task.subscribe(second);
    npos::ipc::Message message;
    message.type = 0x1000;

    task.receiveRequest(message);
    task.process();

    CHECK_EQUAL(1, first.handled.size());
    CHECK_EQUAL(0, second.handled.size());
}

TEST(ServiceTask, ProcessReleasesMessageAndPopsQueue)
{
    task.subscribe(first);
    npos::ipc::Message message;
    message.type = 0x1000;

    task.receiveRequest(message);
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

    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(0, bus.sent.size());
    POINTERS_EQUAL(&message, bus.responded[0]);
}

TEST(ServiceTask, PortIdMatchesAssignedId)
{
    CHECK_EQUAL(6, task.getPortId());
}
