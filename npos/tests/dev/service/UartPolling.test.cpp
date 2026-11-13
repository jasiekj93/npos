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
    npos::ipc::Oid owner;

    void setup() override 
    { 
        service.setId(1);
        owner = { 3, 1 };
    }

    void open()
    {
        npos::dev::msg::Open request("ttyS0");
        request.sender = owner;
        service.handleRequest(request);
        api.responded.clear();
    }

    Transciever read(size_t length = 4, npos::ipc::Oid sender = {0, 0}) 
    { 
        if(sender.portId == 0 && sender.serviceId == 0)
            sender = owner;
        auto msg = Transciever(sender, Transciever::READ, length, etl::span<uint8_t>(rx, sizeof(rx)));
        msg.sender = sender;
        return msg;
    }
    
    Transciever write(size_t length = 4, npos::ipc::Oid sender = {0, 0}) 
    { 
        if(sender.portId == 0 && sender.serviceId == 0)
            sender = owner;
        auto msg = Transciever(sender, Transciever::WRITE, length, etl::span<uint8_t>(tx, sizeof(tx)));
        msg.sender = sender;
        return msg;
    }
    
    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(UartPolling, UnsupportedOperationIsAnsweredWithNotSupported)
{
    open();
    auto request = read();
    request.input.readWrite.mode = 99;

    service.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, statusOfResponse());
}

TEST(UartPolling, AcceptsTranscieverAndOpenMessages)
{
    CHECK_TRUE(service.acceptsRequest(npos::dev::msg::Type::TRANSCIEVER));
    CHECK_TRUE(service.acceptsRequest(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(service.acceptsRequest(npos::dev::msg::Type::LED));
}

TEST(UartPolling, OpenInitializesUartOnceAndAnswersWithOid)
{
    npos::dev::msg::Open request("ttyS0");
    request.sender = owner;

    service.handleRequest(request);
    
    CHECK_EQUAL(1, uart.initCount);
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
    CHECK_EQUAL(1, api.responded[0].output.open.opened.serviceId);
    
    api.responded.clear();
    
    // Second open from same owner should return ALREADY_OPENED
    service.handleRequest(request);
    
    CHECK_EQUAL(1, uart.initCount);  // init not called again
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::ALREADY_OPENED, statusOfResponse());
}

TEST(UartPolling, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    service.handleRequest(request);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartPolling, OpenReportsInitFailure)
{
    uart.failInit = true;
    npos::dev::msg::Open request("ttyS0");
    request.sender = owner;

    service.handleRequest(request);

    CHECK_EQUAL(npos::dev::msg::Open::INIT_FAILURE, statusOfResponse());
}

TEST(UartPolling, TransferBeforeOpenIsRejected)
{
    auto request = read();

    service.handleRequest(request);

    CHECK_EQUAL(Transciever::NOT_INITIALIZED, statusOfResponse());
}

TEST(UartPolling, ReadReceivesIntoRequestBufferAndAnswersAtOnce)
{
    open();
    auto request = read(4, owner);

    service.handleRequest(request);

    POINTERS_EQUAL(rx, uart.received);
    CHECK_EQUAL(4, uart.receivedSize);
    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(4, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, ReadFailureIsReportedWithZeroLength)
{
    open();
    uart.failReceive = true;
    auto request = read(4, owner);

    service.handleRequest(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, WriteTransmitsRequestDataAndAnswersAtOnce)
{
    open();
    auto request = write(6, owner);

    service.handleRequest(request);

    POINTERS_EQUAL(tx, uart.transmitted);
    CHECK_EQUAL(6, uart.transmittedSize);
    CHECK_EQUAL(Transciever::SUCCESS, statusOfResponse());
    CHECK_EQUAL(6, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, WriteFailureIsReported)
{
    open();
    uart.failTransmit = true;
    auto request = write(4, owner);

    service.handleRequest(request);

    CHECK_EQUAL(Transciever::DEVICE_ERROR, statusOfResponse());
    CHECK_EQUAL(0, api.responded[0].output.readWrite.length);
}

TEST(UartPolling, IgnoresOtherMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::LED;

    service.handleRequest(message);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(UartPolling, RejectsOpenFromDifferentOwnerWhenLocked)
{
    open();
    
    npos::dev::msg::Open request("ttyS0");
    request.sender = { 3, 2 };  // different owner
    
    service.handleRequest(request);
    
    CHECK_EQUAL(npos::dev::msg::Open::LOCKED, statusOfResponse());
}

TEST(UartPolling, RejectsTransferFromDifferentOwner)
{
    open();
    
    auto request = read(4, { 3, 2 });  // different owner
    
    service.handleRequest(request);
    
    CHECK_EQUAL(Transciever::LOCKED, statusOfResponse());
}

TEST(UartPolling, AcceptsCloseFromOwner)
{
    open();
    
    npos::dev::msg::Close request(owner);
    request.sender = owner;
    
    service.handleRequest(request);
    
    CHECK_EQUAL(npos::dev::msg::Close::SUCCESS, statusOfResponse());
    CHECK_EQUAL(1, uart.deinitCount);
}

TEST(UartPolling, RejectsCloseFromNonOwner)
{
    open();
    
    npos::dev::msg::Close request(owner);
    request.sender = { 3, 2 };  // different owner
    
    service.handleRequest(request);
    
    CHECK_EQUAL(npos::dev::msg::Close::LOCKED, statusOfResponse());
}

TEST(UartPolling, RejectsCloseWhenNotInitialized)
{
    npos::dev::msg::Close request(owner);
    request.sender = owner;
    
    service.handleRequest(request);
    
    CHECK_EQUAL(npos::dev::msg::Close::NOT_INITIALIZED, statusOfResponse());
}
