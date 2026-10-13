/**
 * @file Api.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/RecordingApi.hpp"

#include <libnpos/dev/Api.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Sender : public npos::ipc::Service
    {
    public:
        using npos::ipc::Service::Service;
        void handle(npos::ipc::Message&) override {}
        bool accepts(npos::ipc::Message::Type) const override { return false; }
    };
}

TEST_GROUP(DevApi)
{
    mock::RecordingApi api;
    Sender sender{api};
    uint8_t buffer[32];
    npos::ipc::Oid device = { 1, 2 };

    const npos::ipc::Message& sent() { return api.sent[0]; }

    void setup() override { sender.setId(4); }
};

TEST(DevApi, OpenBroadcastsDeviceName)
{
    npos::dev::api::open(sender, "uart0");

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Type::OPEN, sent().type);
    CHECK_EQUAL(npos::ipc::NULL_PORT, sent().recipient.portId);
}

TEST(DevApi, GetTimeSendsClockRequest)
{
    npos::dev::api::getTime(sender, device);

    CHECK_EQUAL(npos::dev::msg::Type::CLOCK, sent().type);
    CHECK_EQUAL(npos::dev::msg::Clock::GET_TIME, sent().input.attributes.mode);
}

TEST(DevApi, SetTimeSendsClockRequestWithTimestamp)
{
    npos::dev::api::setTime(sender, device, 1234);

    CHECK_EQUAL(npos::dev::msg::Clock::SET_TIME, sent().input.attributes.mode);
    CHECK_EQUAL(1234, sent().input.attributes.value);
}

TEST(DevApi, LedOnOffToggleSendLedRequests)
{
    npos::dev::api::ledOn(sender, device, 1);
    npos::dev::api::ledOff(sender, device, 2);
    npos::dev::api::ledToggle(sender, device, 3);

    CHECK_EQUAL(3, api.sent.size());
    CHECK_EQUAL(npos::dev::msg::Led::ON, api.sent[0].input.attributes.mode);
    CHECK_EQUAL(npos::dev::msg::Led::OFF, api.sent[1].input.attributes.mode);
    CHECK_EQUAL(npos::dev::msg::Led::TOGGLE, api.sent[2].input.attributes.mode);
    CHECK_EQUAL(3, api.sent[2].input.attributes.value);
}

TEST(DevApi, HashAccumulateSendsInput)
{
    npos::dev::api::hashAccumulate(sender, device, etl::span<const uint8_t>(buffer, 10));

    CHECK_EQUAL(npos::dev::msg::Type::HASH, sent().type);
    CHECK_EQUAL(10, sent().input.size);
}

TEST(DevApi, HashAccumulateEndSendsInputAndOutput)
{
    npos::dev::api::hashAccumulateEnd(sender, device, etl::span<const uint8_t>(buffer, 10), etl::span<uint8_t>(buffer, 32));

    CHECK_EQUAL(npos::dev::msg::Hash::ACCUMULATE_END, sent().input.readWrite.mode);
    CHECK_EQUAL(32, sent().output.size);
}

TEST(DevApi, BackupReadAndWriteSendRegisterRequests)
{
    npos::dev::msg::BackupRegisters::Value value = 0;

    npos::dev::api::backupRead(sender, device, 5, value);
    npos::dev::api::backupWrite(sender, device, 6, value);

    CHECK_EQUAL(npos::dev::msg::Type::BACKUP_REGISTERS, api.sent[0].type);
    CHECK_EQUAL(npos::dev::msg::BackupRegisters::READ, api.sent[0].input.readWrite.mode);
    CHECK_EQUAL(npos::dev::msg::BackupRegisters::WRITE, api.sent[1].input.readWrite.mode);
    CHECK_EQUAL(6, api.sent[1].input.readWrite.offset);
}

TEST(DevApi, TranscieverReadSendsReadRequestWithBuffer)
{
    npos::dev::api::transcieverRead(sender, device, 16, etl::span<uint8_t>(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::dev::msg::Type::TRANSCIEVER, sent().type);
    CHECK_EQUAL(npos::dev::msg::Transciever::READ, sent().input.readWrite.mode);
    CHECK_EQUAL(16, sent().input.readWrite.length);
    POINTERS_EQUAL(buffer, sent().output.data);
}

TEST(DevApi, TranscieverWriteSendsWriteRequestWithData)
{
    npos::dev::api::transcieverWrite(sender, device, etl::span<const uint8_t>(buffer, 12));

    CHECK_EQUAL(npos::dev::msg::Transciever::WRITE, sent().input.readWrite.mode);
    CHECK_EQUAL(12, sent().input.size);
    POINTERS_EQUAL(buffer, sent().input.data);
}

TEST(DevApi, TimerRegisterSendsRequestTargetingTheSender)
{
    npos::dev::api::timerRegister(sender, npos::dev::msg::Timer::REPEATING, 500);

    CHECK_EQUAL(npos::dev::msg::Type::TIMER, sent().type);
    CHECK_EQUAL(3, sent().input.link.target.portId);
    CHECK_EQUAL(4, sent().input.link.target.serviceId);
    CHECK_EQUAL(500, sent().input.link.attributes.value);
}
