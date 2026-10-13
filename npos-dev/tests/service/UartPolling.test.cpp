/**
 * @file UartPolling.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeUartPolling.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/UartPolling.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Transciever;

TEST_GROUP(UartPolling)
{
    mock::RecordingApi api;
    mock::FakeUartPolling uart;
    npos::dev::service::UartPolling service{api, uart, "ttyS0"};
    uint8_t rx[8];
    uint8_t tx[8];

    void setup() override { service.setId(1); }

    void open()
    {
        npos::dev::msg::Open request("ttyS0");
        service.handle(request);
        api.responded.clear();
    }

    Transciever read(size_t length = 4) { return Transciever({ 3, 1 }, Transciever::READ, length, etl::span<uint8_t>(rx, sizeof(rx))); }
    Transciever write(size_t length = 4) { return Transciever({ 3, 1 }, Transciever::WRITE, length, etl::span<uint8_t>(tx, sizeof(tx))); }
    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(UartPolling, AcceptsTranscieverAndOpenMessages)
{
    CHECK_TRUE(service.accepts(npos::dev::msg::Type::TRANSCIEVER));
    CHECK_TRUE(service.accepts(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(service.accepts(npos::dev::msg::Type::LED));
}

TEST(UartPolling, OpenInitializesUartOnceAndAnswersWithOid)
{
    npos::dev::msg::Open request("ttyS0");

    service.handle(request);
    service.handle(request);

    CHECK_EQUAL(1, uart.initCount);
    CHECK_EQUAL(2, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
    CHECK_EQUAL(1, api.responded[0].output.open.opened.serviceId);
}

TEST(UartPolling, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    service.handle(request);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartPolling, OpenReportsInitFailure)
{
    uart.failInit = true;
    npos::dev::msg::Open request("ttyS0");

    service.handle(request);

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, statusOfResponse());
}

TEST(UartPolling, TransferBeforeOpenIsRejected)
{
    auto request = read();

    service.handle(request);

    CHECK_EQUAL(Transciever::NOT_INITIALIZED, statusOfResponse());
}

TEST(UartPolling, ReadReceivesIntoRequestBufferAndAnswersAtOnce)
{
    open();
    auto request = read(4);

    service.handle(request);

    POINTERS_EQUAL(rx, uart.received);
    CHECK_EQUAL(4, uart.receivedSize);
    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(4, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, ReadFailureIsReportedWithZeroLength)
{
    open();
    uart.failReceive = true;
    auto request = read();

    service.handle(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, WriteTransmitsRequestDataAndAnswersAtOnce)
{
    open();
    auto request = write(6);

    service.handle(request);

    POINTERS_EQUAL(tx, uart.transmitted);
    CHECK_EQUAL(6, uart.transmittedSize);
    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(6, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, WriteFailureIsReported)
{
    open();
    uart.failTransmit = true;
    auto request = write();

    service.handle(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, IgnoresOtherMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::LED;

    service.handle(message);

    CHECK_EQUAL(0, api.responded.size());
}
