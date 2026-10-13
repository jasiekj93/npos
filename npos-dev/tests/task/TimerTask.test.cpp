/**
 * @file TimerTask.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBus.hpp"
#include "mock/FakeSystemTick.hpp"

#include <libnpos/dev/task/Timer.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Timer;

TEST_GROUP(TimerTask)
{
    mock::FakeBus bus;
    mock::FakeSystemTick tick;
    npos::dev::task::Timer::MessageQueue<1> queue;
    npos::dev::task::Timer timer{9, queue, bus, tick};

    Timer single(npos::dev::Tick interval) { return Timer({ 2, 1 }, Timer::SINGLE_SHOT, interval); }
};

TEST(TimerTask, KeepsPriority)
{
    CHECK_EQUAL(9, timer.getPriority());
}

TEST(TimerTask, AcceptsOnlyTimerMessages)
{
    CHECK_TRUE(timer.accepts(npos::dev::msg::Type::TIMER));
    CHECK_FALSE(timer.accepts(npos::dev::msg::Type::LED));
}

TEST(TimerTask, IsNotReadyWithoutTimers)
{
    CHECK_FALSE(timer.isReady());
}

TEST(TimerTask, ReceiveIgnoresOtherMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::LED;

    timer.receive(message);

    CHECK_FALSE(timer.isReady());
}

TEST(TimerTask, ReceiveSchedulesEventAndAddressesTheTarget)
{
    tick.tick = 100;
    auto request = single(20);

    timer.receive(request);

    CHECK_EQUAL(120, request.getEventTick());
    CHECK_EQUAL(2, request.recipient.portId);
    CHECK_EQUAL(1, request.recipient.serviceId);
}

TEST(TimerTask, ReceiveWhenFullAnswersNoMemoryAndReleases)
{
    auto first = single(10);
    auto second = single(10);
    timer.receive(first);

    timer.receive(second);

    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(Timer::NO_MEMORY, second.getStatus());
    CHECK_EQUAL(1, bus.released.size());
}

TEST(TimerTask, IsReadyOnceTheIntervalElapsed)
{
    auto request = single(10);
    timer.receive(request);

    CHECK_FALSE(timer.isReady());

    tick.tick = 10;

    CHECK_TRUE(timer.isReady());
}

TEST(TimerTask, ProcessBeforeEventDoesNothing)
{
    auto request = single(10);
    timer.receive(request);

    timer.process();

    CHECK_EQUAL(0, bus.responded.size());
}

TEST(TimerTask, ProcessWithoutTimersDoesNothing)
{
    timer.process();

    CHECK_EQUAL(0, bus.responded.size());
}

TEST(TimerTask, SingleShotTimerAnswersOnceAndIsReleased)
{
    auto request = single(10);
    timer.receive(request);
    tick.tick = 10;

    timer.process();

    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(Timer::SUCCESS, request.getStatus());
    CHECK_EQUAL(1, bus.released.size());
    CHECK_FALSE(timer.isReady());
}

TEST(TimerTask, RepeatingTimerIsRescheduled)
{
    Timer request({ 2, 1 }, Timer::REPEATING, 10);
    timer.receive(request);
    tick.tick = 10;

    timer.process();

    CHECK_EQUAL(1, bus.responded.size());
    CHECK_EQUAL(0, bus.released.size());
    CHECK_EQUAL(20, request.getEventTick());
}
