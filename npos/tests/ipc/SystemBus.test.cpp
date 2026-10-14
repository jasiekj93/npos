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
        message.sender = sender;
        message.recipient = { recipient, npos::ipc::NULL_SERVICE };
        message.type = type;
        return message;
    }
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

TEST(SystemBus, RespondGoesBackToSenderPort)
{
    bus.addPort(first);
    bus.addPort(second);
    auto message = makeMessage(11, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(1, second.received.size());
    CHECK_EQUAL(1, second.received[0]->referenceCount);
}

TEST(SystemBus, RespondToUnknownSenderDoesNothing)
{
    bus.addPort(first);
    auto message = makeMessage(77, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.received.size());
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
    auto message = makeMessage(20, 10);

    bus.respond(message);

    CHECK_EQUAL(0, first.received.size());
    CHECK_EQUAL(1, remote.received.size());
}
