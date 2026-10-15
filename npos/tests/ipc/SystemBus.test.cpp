/**
 * @file SystemBus.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakePort.hpp"

#include <libnpos/ipc/SystemBus.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(SystemBus)
{
    npos::ipc::SystemBus::PortList<4> portList;
    etl::pool<npos::ipc::Message, 4> pool;
    npos::ipc::SystemBus bus{portList, pool, 10};

    mock::FakePort first;
    mock::FakePort second;
    mock::FakePort other{0x2000};

    npos::ipc::Message makeMessage(npos::ipc::PortId sender, npos::ipc::PortId recipient, npos::ipc::Message::Type type = 0x1000)
    {
        npos::ipc::Message message;
        message.sender = { sender, npos::ipc::NULL_SERVICE };
        message.recipient = { recipient, npos::ipc::NULL_SERVICE };
        message.type = type;
        return message;
    }

    npos::ipc::Message makeResponse(npos::ipc::PortId sender, npos::ipc::PortId recipient)
    {
        return makeMessage(sender, recipient);
    }

    // Releases every request right away, the way a port that drops messages does.
    class ReleasingPort : public mock::FakePort
    {
    public:
        explicit ReleasingPort(npos::ipc::Bus& bus) : bus(bus) {}

        void receiveRequest(const npos::ipc::Message& message) override
        {
            FakePort::receiveRequest(message);
            bus.release(const_cast<npos::ipc::Message*>(&message));
        }

        npos::ipc::Bus& bus;
    };
};

TEST(SystemBus, AddPortAssignsConsecutiveIdsFromInitialPort)
{
    CHECK_TRUE(bus.addPort(first));
    CHECK_TRUE(bus.addPort(second));

    CHECK_EQUAL(10, first.getId());
    CHECK_EQUAL(11, second.getId());
}

TEST(SystemBus, AddPortFailsWhenFull)
{
    mock::FakePort a, b, c, d;

    CHECK_TRUE(bus.addPort(a));
    CHECK_TRUE(bus.addPort(b));
    CHECK_TRUE(bus.addPort(c));
    CHECK_TRUE(bus.addPort(d));
    CHECK_FALSE(bus.addPort(first));
}

TEST(SystemBus, SendDeliversPooledCopyToAddressedPort)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeMessage(99, 11);

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(1, second.received.size());
    CHECK_TRUE(pool.is_in_pool(second.received[0]));
    CHECK_EQUAL(1, second.received[0]->referenceCount);
    CHECK_EQUAL(3, pool.available());
}

TEST(SystemBus, SendKeepsMessageAlreadyInPool)
{
    bus.addPort(first);
    auto* pooled = pool.create<npos::ipc::Message>(makeMessage(99, 10));

    bus.send(*pooled);

    POINTERS_EQUAL(pooled, first.received[0]);
    CHECK_EQUAL(1, pooled->referenceCount);
    CHECK_EQUAL(3, pool.available());
}

TEST(SystemBus, SendToUnknownPortWithoutSuccessorDoesNothing)
{
    bus.addPort(first);
    auto message = makeMessage(99, 30);

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, SendToUnknownPortAnswersPortNotFoundToTheSender)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeMessage(11, 30);
    message.output.status = 0;

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(0, second.received.size());
    CHECK_EQUAL(1, second.receivedResponses.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_PORT_NOT_FOUND, second.receivedResponses[0]->output.status);
}

TEST(SystemBus, SendToPortThatDoesNotAcceptTheTypeIsPassedToItsUnknownRequestHandler)
{
    bus.addPort(first);
    bus.addPort(other);
    auto message = makeMessage(10, 11, 0x1000);

    bus.send(message);

    CHECK_EQUAL(0, other.received.size());
    CHECK_EQUAL(1, other.unknown.size());
    POINTERS_EQUAL(&message, other.unknown[0]);
    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, SendToPortBelowInitialPortDoesNothing)
{
    bus.addPort(first);
    auto message = makeMessage(99, 2);

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
}

TEST(SystemBus, BroadcastReachesOnlyAcceptingPortsExceptSender)
{
    bus.addPort(first);
    bus.addPort(second);
    bus.addPort(other);
    auto message = makeMessage(10, npos::ipc::NULL_PORT);

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(1, second.received.size());
    CHECK_EQUAL(0, other.received.size());
    CHECK_EQUAL(1, second.received[0]->referenceCount);
}

TEST(SystemBus, BroadcastCountsReferenceForEveryReceiver)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeMessage(99, npos::ipc::NULL_PORT);

    bus.send(message);

    POINTERS_EQUAL(first.received[0], second.received[0]);
    CHECK_EQUAL(2, first.received[0]->referenceCount);
}

TEST(SystemBus, BroadcastWithoutReceiversDoesNotLeakPoolMessage)
{
    bus.addPort(other);
    auto message = makeMessage(99, npos::ipc::NULL_PORT);

    bus.send(message);

    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, BroadcastIsNotAnsweredWhenNobodyAccepts)
{
    bus.addPort(first);
    bus.addPort(other);
    auto message = makeMessage(10, npos::ipc::NULL_PORT, 0x3000);

    bus.send(message);

    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(0, other.unknown.size());
}

TEST(SystemBus, BroadcastSurvivesPortThatReleasesTheMessageImmediately)
{
    ReleasingPort dropping{bus};
    bus.addPort(dropping);
    bus.addPort(first);
    auto message = makeMessage(99, npos::ipc::NULL_PORT);

    bus.send(message);

    CHECK_EQUAL(1, dropping.received.size());
    CHECK_EQUAL(1, first.received.size());
    POINTERS_EQUAL(dropping.received[0], first.received[0]);
    CHECK_EQUAL(1, first.received[0]->referenceCount);
    CHECK_EQUAL(3, pool.available());
}

TEST(SystemBus, CopyToPoolStartsWithOneReference)
{
    bus.addPort(first);
    auto message = makeMessage(99, 10);
    message.referenceCount = 5;

    bus.send(message);

    CHECK_EQUAL(1, first.received[0]->referenceCount);
}

TEST(SystemBus, SendWithExhaustedPoolDropsTheMessage)
{
    bus.addPort(first);
    npos::ipc::Message* held[4];
    for(auto& slot : held)
        slot = pool.create<npos::ipc::Message>(makeMessage(0, 0));
    auto message = makeMessage(99, 10);
    auto response = makeResponse(10, 99);

    bus.send(message);
    bus.respond(response);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(0, pool.available());
}

TEST(SystemBus, RespondGoesBackToSenderPort)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeResponse(11, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(1, second.receivedResponses.size());
    CHECK_EQUAL(0, second.received.size());
    CHECK_EQUAL(1, second.receivedResponses[0]->referenceCount);
}

TEST(SystemBus, ResponseIsRoutedBySenderNotByRecipient)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeResponse(10, 11);

    bus.respond(message);

    CHECK_EQUAL(1, first.receivedResponses.size());
    CHECK_EQUAL(0, second.receivedResponses.size());
}

TEST(SystemBus, ResponseIsNotBroadcast)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeResponse(10, npos::ipc::NULL_PORT);

    bus.respond(message);

    CHECK_EQUAL(1, first.receivedResponses.size());
    CHECK_EQUAL(0, second.receivedResponses.size());
}

TEST(SystemBus, RespondKeepsMessageAlreadyInPool)
{
    bus.addPort(first);
    auto* pooled = pool.create<npos::ipc::Message>(makeResponse(10, 99));
    pooled->referenceCount = 1;

    bus.respond(*pooled);

    POINTERS_EQUAL(pooled, first.receivedResponses[0]);
    CHECK_EQUAL(2, pooled->referenceCount);
    CHECK_EQUAL(3, pool.available());
}

TEST(SystemBus, RespondToUnknownSenderDoesNothing)
{
    bus.addPort(first);
    auto message = makeResponse(77, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, ReleaseReturnsMessageToPoolWhenLastReferenceDropped)
{
    bus.addPort(first);
    auto message = makeMessage(99, 10);
    bus.send(message);
    auto* pooled = const_cast<npos::ipc::Message*>(first.received[0]);

    bus.release(pooled);

    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, ReleaseKeepsMessageWhileReferencesRemain)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeMessage(99, npos::ipc::NULL_PORT);
    bus.send(message);
    auto* pooled = const_cast<npos::ipc::Message*>(first.received[0]);

    bus.release(pooled);

    CHECK_EQUAL(3, pool.available());
    CHECK_EQUAL(1, pooled->referenceCount);

    bus.release(pooled);

    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, ReleaseIgnoresNullAndForeignMessages)
{
    npos::ipc::Message foreign;

    bus.release(nullptr);
    bus.release(&foreign);

    CHECK_EQUAL(4, pool.available());
}

TEST(SystemBus, SendToUnknownPortIsForwardedToSuccessor)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote;
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    auto message = makeMessage(10, 20);

    bus.send(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(1, remote.received.size());
}

TEST(SystemBus, BroadcastReachesSuccessorPorts)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote;
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    auto message = makeMessage(99, npos::ipc::NULL_PORT);

    bus.send(message);

    CHECK_EQUAL(1, first.received.size());
    CHECK_EQUAL(1, remote.received.size());
}

TEST(SystemBus, RespondIsForwardedToSuccessorWhenSenderIsRemote)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote;
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    auto message = makeResponse(20, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.receivedResponses.size());
    CHECK_EQUAL(1, remote.receivedResponses.size());
    CHECK_TRUE(successorPool.is_in_pool(remote.receivedResponses[0]));
    CHECK_EQUAL(1, remote.receivedResponses[0]->referenceCount);
}

TEST(SystemBus, SuccessorsPointingAtEachOtherDoNotLoopOnUnknownPort)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote;
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    successor.setSuccesor(bus);
    auto message = makeMessage(99, 77);

    bus.send(message);
    bus.respond(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(0, remote.received.size());
}

TEST(SystemBus, RequestRejectedBySuccessorPortReachesItsUnknownRequestHandler)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote{0x2000};
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    successor.setSuccesor(bus);
    auto message = makeMessage(10, 20, 0x1000);

    bus.send(message);

    CHECK_EQUAL(0, remote.received.size());
    CHECK_EQUAL(1, remote.unknown.size());
}

TEST(SystemBus, PortNotFoundIsAnsweredAcrossBuses)
{
    npos::ipc::SystemBus::PortList<2> successorPorts;
    etl::pool<npos::ipc::Message, 2> successorPool;
    npos::ipc::SystemBus successor(successorPorts, successorPool, 20);
    mock::FakePort remote;
    successor.addPort(remote);
    bus.addPort(first);
    bus.setSuccesor(successor);
    successor.setSuccesor(bus);
    auto message = makeMessage(20, 77);

    successor.send(message);

    CHECK_EQUAL(1, remote.receivedResponses.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_PORT_NOT_FOUND, remote.receivedResponses[0]->output.status);
}
