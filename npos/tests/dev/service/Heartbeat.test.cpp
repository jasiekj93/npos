/**
 * @file Heartbeat.test.cpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/Heartbeat.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Open;
using npos::dev::msg::Timer;
using npos::dev::msg::Led;

TEST_GROUP(Heartbeat)
{
    mock::RecordingApi api;
    npos::dev::service::Heartbeat heartbeat{api, "leds", 1, 500};

    void setup() override { heartbeat.setId(2); }

    void opened(Open::Status status = Open::SUCCESS)
    {
        Open response("leds");
        response.setStatus(status).setOpened({ 4, 7 });
        heartbeat.handleResponse(response);
    }

    void timer(Timer::Status status = Timer::SUCCESS)
    {
        Timer response({ 3, 2 }, Timer::REPEATING, 500);
        response.setStatus(status);
        heartbeat.handleResponse(response);
    }
};

TEST(Heartbeat, AcceptsNoRequests)
{
    CHECK_FALSE(heartbeat.acceptsRequest(npos::dev::msg::Type::LED));
    CHECK_FALSE(heartbeat.acceptsRequest(npos::dev::msg::Type::OPEN));
}

TEST(Heartbeat, InitializeOpensTheLedDevice)
{
    heartbeat.initialize();

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::OPEN, api.sent[0].type);
    CHECK_EQUAL(3, api.sent[0].sender.portId);
    CHECK_EQUAL(2, api.sent[0].sender.serviceId);
    CHECK_TRUE(reinterpret_cast<Open&>(api.sent[0]).getName() == "leds");
}

TEST(Heartbeat, OpenedDeviceStartsARepeatingTimerOfTheInterval)
{
    opened();

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::TIMER, api.sent[0].type);
    auto& request = reinterpret_cast<Timer&>(api.sent[0]);
    CHECK_EQUAL(Timer::REPEATING, request.getMode());
    CHECK_EQUAL(500, request.getInterval());
    CHECK_EQUAL(3, request.getTarget().portId);
    CHECK_EQUAL(2, request.getTarget().serviceId);
}

TEST(Heartbeat, FailedOpenStartsNoTimer)
{
    opened(Open::NOT_FOUND);
    opened(Open::INIT_FAILURE);

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Heartbeat, SecondOpenResponseDoesNotStartAnotherTimer)
{
    opened();
    opened();

    CHECK_EQUAL(1, api.sent.size());
}

TEST(Heartbeat, TimerTogglesTheConfiguredLedOfTheOpenedDevice)
{
    opened();
    api.sent.clear();

    timer();

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::LED, api.sent[0].type);
    CHECK_EQUAL(4, api.sent[0].recipient.portId);
    CHECK_EQUAL(7, api.sent[0].recipient.serviceId);
    auto& request = reinterpret_cast<Led&>(api.sent[0]);
    CHECK_EQUAL(1, request.getId());
    CHECK_EQUAL(Led::TOGGLE, request.getOperation());
}

TEST(Heartbeat, EveryTimerEventTogglesTheLed)
{
    opened();
    api.sent.clear();

    timer();
    timer();
    timer();

    CHECK_EQUAL(3, api.sent.size());
}

TEST(Heartbeat, FailedTimerDoesNotToggle)
{
    opened();
    api.sent.clear();

    timer(Timer::NO_MEMORY);

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Heartbeat, TimerBeforeTheDeviceIsOpenedDoesNotToggle)
{
    timer();

    CHECK_EQUAL(0, api.sent.size());
}

TEST(Heartbeat, LedAnswersAreIgnored)
{
    opened();
    api.sent.clear();
    Led response({ 4, 7 }, 1, Led::TOGGLE);
    response.setStatus(Led::SUCCESS);

    heartbeat.handleResponse(response);

    CHECK_EQUAL(0, api.sent.size());
}
