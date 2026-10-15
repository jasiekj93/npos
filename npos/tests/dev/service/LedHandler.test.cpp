/**
 * @file LedHandler.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeLed.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/LedHandler.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(LedHandler)
{
    mock::RecordingApi api;
    mock::FakeLed led0, led1;
    etl::vector<npos::dev::hal::Led*, 2> leds { &led0, &led1 };
    npos::dev::service::LedHandler handler{api, leds, "leds"};

    void setup() override { handler.setId(1); }

    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(LedHandler, AcceptsLedAndOpenMessages)
{
    CHECK_TRUE(handler.acceptsRequest(npos::dev::msg::Type::LED));
    CHECK_TRUE(handler.acceptsRequest(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(handler.acceptsRequest(npos::dev::msg::Type::CLOCK));
}

TEST(LedHandler, OpenWithItsNameAnswersWithItsOid)
{
    npos::dev::msg::Open request("leds");

    handler.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
    CHECK_EQUAL(3, api.responded[0].output.open.opened.portId);
    CHECK_EQUAL(1, api.responded[0].output.open.opened.serviceId);
}

TEST(LedHandler, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    handler.handleRequest(request);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(LedHandler, SwitchesLedsOnOffAndToggles)
{
    npos::dev::msg::Led on({ 3, 1 }, 1, npos::dev::msg::Led::ON);
    npos::dev::msg::Led off({ 3, 1 }, 1, npos::dev::msg::Led::OFF);
    npos::dev::msg::Led toggle({ 3, 1 }, 0, npos::dev::msg::Led::TOGGLE);

    handler.handleRequest(on);
    handler.handleRequest(toggle);
    handler.handleRequest(off);

    CHECK_EQUAL(1, led1.onCount);
    CHECK_EQUAL(1, led1.offCount);
    CHECK_EQUAL(1, led0.toggleCount);
    CHECK_EQUAL(3, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Led::SUCCESS, statusOfResponse());
}

TEST(LedHandler, UnsupportedOperationIsAnsweredWithNotSupported)
{
    npos::dev::msg::Led request({ 3, 1 }, 0, npos::dev::msg::Led::ON);
    request.input.attributes.mode = 99;

    handler.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, statusOfResponse());
    CHECK_EQUAL(0, led0.onCount + led0.offCount + led0.toggleCount);
}

TEST(LedHandler, UnknownLedIsReported)
{
    npos::dev::msg::Led request({ 3, 1 }, 5, npos::dev::msg::Led::ON);

    handler.handleRequest(request);

    CHECK_EQUAL(npos::dev::msg::Led::LED_NOT_FOUND, statusOfResponse());
    CHECK_EQUAL(0, led0.onCount + led1.onCount);
}

TEST(LedHandler, IgnoresOtherMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::CLOCK;

    handler.handleRequest(message);

    CHECK_EQUAL(0, api.responded.size());
}
