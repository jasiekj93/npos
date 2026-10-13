/**
 * @file UartInterrupt.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeUart.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/UartInterrupt.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::Transciever;
using npos::dev::hal::Interrupt;
using npos::dev::hal::UartInterrupt;

TEST_GROUP(UartInterrupt)
{
    mock::RecordingApi api;
    mock::FakeUart uart;
    npos::dev::InterruptService::InterruptQueue<4> queue;
    npos::dev::service::UartInterrupt service{api, queue, uart, "uart0"};
    uint8_t rx[8];
    uint8_t tx[8];

    void setup() override { service.setId(1); }

    void open()
    {
        npos::dev::msg::Open request("uart0");
        service.handle(request);
        api.responded.clear();
    }

    void interrupt(npos::dev::hal::Interrupt::Code code, Interrupt::Type type = Interrupt::UART)
    {
        uart.fire(type, code);
        service.processInterrupt();
    }

    Transciever read(size_t length = 4) { return Transciever({ 3, 1 }, Transciever::READ, length, etl::span<uint8_t>(rx, sizeof(rx))); }
    Transciever write(size_t length = 4) { return Transciever({ 3, 1 }, Transciever::WRITE, length, etl::span<uint8_t>(tx, sizeof(tx))); }
    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(UartInterrupt, AcceptsTranscieverAndOpenMessages)
{
    CHECK_TRUE(service.accepts(npos::dev::msg::Type::TRANSCIEVER));
    CHECK_TRUE(service.accepts(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(service.accepts(npos::dev::msg::Type::LED));
}

TEST(UartInterrupt, OpenInitializesUartOnceAndAnswersWithOid)
{
    npos::dev::msg::Open request("uart0");

    service.handle(request);
    service.handle(request);

    CHECK_EQUAL(1, uart.initCount);
    CHECK_EQUAL(2, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
}

TEST(UartInterrupt, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("uart1");

    service.handle(request);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartInterrupt, OpenReportsInitFailure)
{
    uart.failInit = true;
    npos::dev::msg::Open request("uart0");

    service.handle(request);

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, statusOfResponse());
}

TEST(UartInterrupt, TransferBeforeOpenIsRejected)
{
    auto request = read();

    service.handle(request);

    CHECK_EQUAL(Transciever::NOT_INITIALIZED, statusOfResponse());
}

TEST(UartInterrupt, ReadStartsReceptionIntoRequestBuffer)
{
    open();
    auto request = read(4);

    service.handle(request);

    POINTERS_EQUAL(rx, uart.received);
    CHECK_EQUAL(4, uart.receivedSize);
    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(1, request.referenceCount);
}

TEST(UartInterrupt, ReceiveCompleteAnswersWithLength)
{
    open();
    auto request = read(4);
    service.handle(request);

    interrupt(UartInterrupt::RECEIVE_COMPLETE);

    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(4, api.responded[0].output.readWrite.length);
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(UartInterrupt, SecondReadWhileBusyIsRejected)
{
    open();
    auto first = read();
    auto second = read();
    service.handle(first);

    service.handle(second);

    CHECK_EQUAL(Transciever::BUSY, statusOfResponse());
    CHECK_EQUAL(0, api.responded[0].output.readWrite.length);
}

TEST(UartInterrupt, ReadFailureIsReportedAndFreesTheSlot)
{
    open();
    uart.failReceive = true;
    auto request = read();

    service.handle(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);

    uart.failReceive = false;
    auto again = read();
    service.handle(again);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(1, again.referenceCount);
}

TEST(UartInterrupt, WriteStartsTransmission)
{
    open();
    auto request = write(6);

    service.handle(request);

    POINTERS_EQUAL(tx, uart.transmitted);
    CHECK_EQUAL(6, uart.transmittedSize);
    CHECK_EQUAL(1, request.referenceCount);
}

TEST(UartInterrupt, TransmitCompleteAnswersWithLength)
{
    open();
    auto request = write(6);
    service.handle(request);

    interrupt(UartInterrupt::TRANSMIT_COMPLETE);

    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(6, api.responded[0].output.readWrite.length);
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(UartInterrupt, SecondWriteWhileBusyIsRejected)
{
    open();
    auto first = write();
    auto second = write();
    service.handle(first);

    service.handle(second);

    CHECK_EQUAL(Transciever::BUSY, statusOfResponse());
}

TEST(UartInterrupt, WriteFailureIsReported)
{
    open();
    uart.failTransmit = true;
    auto request = write();

    service.handle(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(UartInterrupt, ReadAndWriteCanRunTogether)
{
    open();
    auto reading = read();
    auto writing = write();
    service.handle(reading);
    service.handle(writing);

    interrupt(UartInterrupt::TRANSMIT_COMPLETE);
    interrupt(UartInterrupt::RECEIVE_COMPLETE);

    CHECK_EQUAL(2, api.responded.size());
    CHECK_EQUAL(Transciever::SUCCESS, api.responded[0].output.status);
    CHECK_EQUAL(Transciever::SUCCESS, api.responded[1].output.status);
}

TEST(UartInterrupt, ErrorInterruptFailsPendingRead)
{
    open();
    auto request = read();
    service.handle(request);

    interrupt(UartInterrupt::ERROR);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(UartInterrupt, ErrorInterruptFailsPendingWriteWhenNoRead)
{
    open();
    auto request = write();
    service.handle(request);

    interrupt(UartInterrupt::ERROR);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, request.referenceCount);
}

TEST(UartInterrupt, CompletionWithoutRequestIsIgnored)
{
    open();

    interrupt(UartInterrupt::TRANSMIT_COMPLETE);
    interrupt(UartInterrupt::RECEIVE_COMPLETE);
    interrupt(UartInterrupt::ERROR);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartInterrupt, InterruptFromOtherPeripheralIsIgnored)
{
    open();
    auto request = write();
    service.handle(request);

    interrupt(UartInterrupt::TRANSMIT_COMPLETE, Interrupt::HASH);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartInterrupt, UnknownEventIsIgnored)
{
    open();
    auto request = write();
    service.handle(request);

    interrupt(0x7F);

    CHECK_EQUAL(0, api.responded.size());
}
