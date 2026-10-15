/**
 * @file FsmService.test.cpp
 * @author Adrian Szczepanski
 * @date 15-10-2026
 */

#include "mock/RecordingApi.hpp"

#include <libnpos/ipc/FsmService.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    using npos::ipc::FsmService;
    using npos::ipc::Message;

    class TestState : public FsmService::State
    {
    public:
        TestState(ID id, FsmService& fsm, ID next = NO_STATE_CHANGE)
            : State(id, fsm)
            , next(next)
        {
        }

        void onEnter() override { entered++; }
        void onExit() override { exited++; }
        bool acceptsRequest(Message::Type type) const override { return type == 0x1000; }
        ID handleRequest(Message&) override { requests++; return next; }
        ID handleResponse(Message&) override { responses++; return next; }
        ID handleUnknownRequest(Message&) override { unknown++; return next; }

        ID next;
        int entered = 0;
        int exited = 0;
        int requests = 0;
        int responses = 0;
        int unknown = 0;
    };
}

TEST_GROUP(FsmService)
{
    mock::RecordingApi api;
    FsmService::State* states[2] = { nullptr, nullptr };
    FsmService fsm{api, states};
    TestState first{0, fsm, 1};
    TestState second{1, fsm};

    void setup() override
    {
        states[0] = &first;
        states[1] = &second;
    }
};

TEST(FsmService, IgnoresEverythingUntilStarted)
{
    Message message;
    message.type = 0x1000;

    fsm.handleRequest(message);
    fsm.handleResponse(message);
    fsm.handleUnknownRequest(message);

    CHECK_FALSE(fsm.acceptsRequest(0x1000));
    CHECK_EQUAL(0, first.requests);
    CHECK_EQUAL(0, first.responses);
    CHECK_EQUAL(0, first.unknown);
}

TEST(FsmService, StartEntersTheFirstState)
{
    fsm.start();

    POINTERS_EQUAL(&first, fsm.getCurrentState());
    CHECK_EQUAL(1, first.entered);
}

TEST(FsmService, StartWithoutStatesDoesNothing)
{
    FsmService empty{api, nullptr};

    empty.start();

    CHECK_TRUE(empty.getCurrentState() == nullptr);
    CHECK_FALSE(empty.acceptsRequest(0x1000));
}

TEST(FsmService, AcceptsWhatTheCurrentStateAccepts)
{
    fsm.start();

    CHECK_TRUE(fsm.acceptsRequest(0x1000));
    CHECK_FALSE(fsm.acceptsRequest(0x2000));
}

TEST(FsmService, RequestIsHandledByCurrentStateWhenAccepted)
{
    fsm.start();
    Message message;
    message.type = 0x1000;

    fsm.handleRequest(message);

    CHECK_EQUAL(1, first.requests);
    POINTERS_EQUAL(&second, fsm.getCurrentState());
    CHECK_EQUAL(1, first.exited);
    CHECK_EQUAL(1, second.entered);
}

TEST(FsmService, RequestTheStateDoesNotAcceptIsHandledAsUnknown)
{
    fsm.start();
    Message message;
    message.type = 0x2000;

    fsm.handleRequest(message);

    CHECK_EQUAL(0, first.requests);
    CHECK_EQUAL(1, first.unknown);
}

TEST(FsmService, UnknownRequestIsAnsweredWithNotSupportedWhenNotRunning)
{
    Message message;
    message.output.status = 0;

    fsm.handleRequest(message);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, api.responded[0].output.status);
    CHECK_EQUAL(0, first.unknown);
}

TEST(FsmService, StateWithoutUnknownRequestHandlerAnswersNotSupported)
{
    class PlainState : public FsmService::State
    {
    public:
        PlainState(FsmService& fsm) : State(0, fsm) {}
    };

    FsmService::State* plainStates[1] = { nullptr };
    FsmService other{api, plainStates};
    PlainState plain{other};
    plainStates[0] = &plain;
    other.start();
    Message message;
    message.output.status = 0;

    other.handleUnknownRequest(message);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, api.responded[0].output.status);
}

TEST(FsmService, ResponseIsHandledByCurrentState)
{
    fsm.start();
    Message message;

    fsm.handleResponse(message);

    CHECK_EQUAL(1, first.responses);
    POINTERS_EQUAL(&second, fsm.getCurrentState());
}

TEST(FsmService, UnknownRequestIsHandledByCurrentState)
{
    fsm.start();
    Message message;
    message.type = 0x2000;

    fsm.handleUnknownRequest(message);

    CHECK_EQUAL(1, first.unknown);
    CHECK_EQUAL(0, first.requests);
    POINTERS_EQUAL(&second, fsm.getCurrentState());
}

TEST(FsmService, StateReturningNoStateChangeStays)
{
    fsm.start();
    fsm.setState(1);
    Message message;

    fsm.handleResponse(message);

    CHECK_EQUAL(1, second.responses);
    POINTERS_EQUAL(&second, fsm.getCurrentState());
    CHECK_EQUAL(1, second.entered);
    CHECK_EQUAL(0, second.exited);
}
