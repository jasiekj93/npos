/**
 * @file Date.test.cpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/cli/program/Date.hpp>
#include <libnpos/fs/msg/Type.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Open;
using npos::dev::msg::Clock;
using npos::dev::msg::Timer;

TEST_GROUP(Date)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Date date{api, "clock"};

    // Pops the oldest request of the given type, the clock and timer requests are sent in this order.
    npos::ipc::Message take(npos::ipc::Message::Type type)
    {
        for(size_t i = 0; i < api.sent.size(); i++)
        {
            if(api.sent[i].type == type)
            {
                auto message = api.sent[i];
                api.sent.erase(api.sent.begin() + i);
                return message;
            }
        }

        FAIL("no such request");
        return npos::ipc::Message();
    }

    void answerOpen(Open::Status status = Open::SUCCESS, npos::ipc::Oid opened = { 4, 1 })
    {
        auto message = take(npos::dev::msg::Type::OPEN);
        reinterpret_cast<Open&>(message).setStatus(status).setOpened(opened);
        date.handleResponse(message);
    }

    void timerFires()
    {
        Timer response({ 3, 0 }, Timer::SINGLE_SHOT, 1000);
        response.setStatus(Timer::SUCCESS);
        date.handleResponse(response);
    }

    void answerClock(Clock::Status status, npos::dev::Timestamp timestamp = 0)
    {
        auto message = take(npos::dev::msg::Type::CLOCK);
        reinterpret_cast<Clock&>(message).setStatus(status).setOutputTimestamp(timestamp);
        date.handleResponse(message);
    }

    void openAndShow()
    {
        date.start(host, "");
        answerOpen();
        answerClock(Clock::SUCCESS, 1790686224);
    }

    void assertUsage()
    {
        CHECK_TRUE(host.outputIs("usage: date [-s YYYY-MM-DD [HH:MM[:SS]]]\r\n"));
        CHECK_EQUAL(1, host.exitCode);
        CHECK_EQUAL(0, api.sent.size());
    }
};

TEST(Date, HasNameAndDescription)
{
    CHECK_TRUE(date.getName() == "date");
    CHECK_TRUE(date.getDescription().size() > 0);
}

TEST(Date, StartOpensTheClockAndRegistersAnOpenTimeout)
{
    date.start(host, "");

    CHECK_EQUAL(2, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::OPEN, api.sent[0].type);
    CHECK_TRUE(reinterpret_cast<Open&>(api.sent[0]).getName() == "clock");
    CHECK_EQUAL(npos::dev::msg::Type::TIMER, api.sent[1].type);
    auto& timer = reinterpret_cast<Timer&>(api.sent[1]);
    CHECK_EQUAL(Timer::SINGLE_SHOT, timer.getMode());
    CHECK_EQUAL(npos::cli::program::Date::OPEN_TIMEOUT, timer.getInterval());
    CHECK_EQUAL(0, host.exitCount);
}

TEST(Date, OpenedClockIsAskedForTheTime)
{
    date.start(host, "");

    answerOpen(Open::SUCCESS, { 4, 1 });

    auto request = take(npos::dev::msg::Type::CLOCK);
    CHECK_EQUAL(4, request.recipient.portId);
    CHECK_EQUAL(1, request.recipient.serviceId);
    CHECK_EQUAL(Clock::GET_TIME, reinterpret_cast<Clock&>(request).getOperation());
}

TEST(Date, PrintsTheTimeOfTheClock)
{
    openAndShow();

    CHECK_TRUE(host.outputIs("2026-09-29 12:50:24\r\n"));
    CHECK_EQUAL(0, host.exitCode);
    POINTERS_EQUAL(&date, host.exited);
}

TEST(Date, ClockOpenedBeforeIsNotOpenedAgain)
{
    openAndShow();
    host.output.clear();
    api.sent.clear();

    date.start(host, "");

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::CLOCK, api.sent[0].type);
    CHECK_EQUAL(4, api.sent[0].recipient.portId);

    answerClock(Clock::SUCCESS, 946684800);

    CHECK_TRUE(host.outputIs("2000-01-01 00:00:00\r\n"));
    CHECK_EQUAL(2, host.exitCount);
}

TEST(Date, FailedOpenIsReported)
{
    date.start(host, "");

    answerOpen(Open::INIT_FAILURE);

    CHECK_TRUE(host.outputIs("date: cannot open clock\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Date, MissingClockEndsWithTheTimeout)
{
    date.start(host, "");

    timerFires();

    CHECK_TRUE(host.outputIs("date: clock: no such device\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Date, TimeoutAfterTheClockAnsweredIsIgnored)
{
    date.start(host, "");
    answerOpen();

    timerFires();

    CHECK_EQUAL(0, host.exitCount);

    answerClock(Clock::SUCCESS, 1790686224);

    CHECK_EQUAL(1, host.exitCount);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Date, OpenAnswerAfterTheTimeoutIsIgnored)
{
    date.start(host, "");
    timerFires();
    host.output.clear();
    auto open = take(npos::dev::msg::Type::OPEN);
    reinterpret_cast<Open&>(open).setStatus(Open::SUCCESS).setOpened({ 4, 1 });

    date.handleResponse(open);

    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Date, TimerOfAnEarlierRunDoesNotCancelTheNextOne)
{
    date.start(host, "");
    answerOpen(Open::INIT_FAILURE);
    host.output.clear();
    api.sent.clear();

    date.start(host, "");
    timerFires();

    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));

    timerFires();

    CHECK_EQUAL(2, host.exitCount);
    CHECK_TRUE(host.outputIs("date: clock: no such device\r\n"));
}

TEST(Date, LateTimerDoesNotDisturbTheNextRun)
{
    openAndShow();
    host.output.clear();

    date.start(host, "");
    timerFires();

    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Date, SetsTheClockFromDateAndTime)
{
    date.start(host, "-s 2026-09-29 12:50:24");
    answerOpen();

    auto request = take(npos::dev::msg::Type::CLOCK);
    auto& clock = reinterpret_cast<Clock&>(request);
    CHECK_EQUAL(Clock::SET_TIME, clock.getOperation());
    CHECK_EQUAL(1790686224, clock.getInputTimestamp());
}

TEST(Date, PrintsTheTimeItHasSet)
{
    date.start(host, "-s 2026-09-29 12:50:24");
    answerOpen();

    answerClock(Clock::SUCCESS, 1790686224);

    CHECK_TRUE(host.outputIs("2026-09-29 12:50:24\r\n"));
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Date, SetsMidnightWhenOnlyTheDateIsGiven)
{
    date.start(host, "-s 2024-02-29");
    answerOpen();

    auto request = take(npos::dev::msg::Type::CLOCK);
    CHECK_EQUAL(1709164800, reinterpret_cast<Clock&>(request).getInputTimestamp());
}

TEST(Date, SetsAlreadyOpenedClockWithoutOpeningIt)
{
    openAndShow();
    host.output.clear();
    api.sent.clear();

    date.start(host, "-s 2026-09-29 12:50");

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(Clock::SET_TIME, reinterpret_cast<Clock&>(api.sent[0]).getOperation());
}

TEST(Date, UsageWhenTheOptionIsUnknown)
{
    date.start(host, "-x");

    assertUsage();
}

TEST(Date, UsageWhenNoDateFollowsTheOption)
{
    date.start(host, "-s");

    assertUsage();
}

TEST(Date, UsageWhenTheDateIsMalformed)
{
    date.start(host, "-s yesterday");

    assertUsage();
}

TEST(Date, UsageWhenTheDateDoesNotExist)
{
    date.start(host, "-s 2026-02-30 10:00");

    assertUsage();
}

TEST(Date, UsageWhenTheDateIsWithoutOption)
{
    date.start(host, "2026-09-29");

    assertUsage();
}

TEST(Date, ReportsADateTheClockCannotHold)
{
    date.start(host, "-s 1980-01-01");
    answerOpen();

    answerClock(Clock::INVALID_TIMESTAMP);

    CHECK_TRUE(host.outputIs("date: date out of the supported range\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Date, ReportsClockFailures)
{
    date.start(host, "");
    answerOpen();

    answerClock(Clock::DEVICE_FAILURE);

    CHECK_TRUE(host.outputIs("date: clock failure\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Date, ReportsAClockThatIsNotInitialized)
{
    date.start(host, "");
    answerOpen();

    answerClock(Clock::NOT_INITIALIZED);

    CHECK_TRUE(host.outputIs("date: clock not initialized\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Date, IgnoresClockAnswersWhenIdle)
{
    Clock response({ 4, 1 }, Clock::GET_TIME);
    response.setStatus(Clock::SUCCESS);

    date.handleResponse(response);

    CHECK_EQUAL(0, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Date, IgnoresUnrelatedAnswers)
{
    date.start(host, "");
    npos::ipc::Message other;
    other.type = npos::fs::msg::Type::FILE_OPEN;

    date.handleResponse(other);

    CHECK_EQUAL(0, host.exitCount);
}
