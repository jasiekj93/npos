/**
 * @file BackupStore.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBackupRegisters.hpp"
#include "mock/RecordingApi.hpp"

#include <libnpos/dev/service/BackupStore.hpp>

#include <CppUTest/TestHarness.h>

using npos::dev::msg::BackupRegisters;

TEST_GROUP(BackupStore)
{
    mock::RecordingApi api;
    mock::FakeBackupRegisters registers;
    npos::dev::service::BackupStore store{api, registers, "backup"};
    BackupRegisters::Value value = 0;
    npos::ipc::Oid owner;

    void setup() override 
    { 
        store.setId(2);
        owner = { 3, 2 };
    }

    void open()
    {
        npos::dev::msg::Open request("backup");
        request.sender = owner;
        store.handleRequest(request);
        api.responded.clear();
    }

    BackupRegisters read(int index = 0, npos::ipc::Oid sender = {0, 0})
    {
        if(sender.portId == 0 && sender.serviceId == 0)
            sender = owner;
        auto msg = BackupRegisters(sender, BackupRegisters::READ, index, &value);
        msg.sender = sender;
        return msg;
    }

    BackupRegisters write(int index = 0, npos::ipc::Oid sender = {0, 0})
    {
        if(sender.portId == 0 && sender.serviceId == 0)
            sender = owner;
        auto msg = BackupRegisters(sender, BackupRegisters::WRITE, index, &value);
        msg.sender = sender;
        return msg;
    }

    uint8_t statusOfResponse() { return api.responded[0].output.status; }
};

TEST(BackupStore, UnsupportedOperationIsAnsweredWithNotSupported)
{
    open();
    auto request = read();
    request.input.readWrite.mode = 99;

    store.handleRequest(request);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED, statusOfResponse());
}

TEST(BackupStore, AcceptsRegisterAndOpenMessages)
{
    CHECK_TRUE(store.acceptsRequest(npos::dev::msg::Type::BACKUP_REGISTERS));
    CHECK_TRUE(store.acceptsRequest(npos::dev::msg::Type::OPEN));
    CHECK_FALSE(store.acceptsRequest(npos::dev::msg::Type::LED));
}

TEST(BackupStore, OpenEnablesAccessOnceAndAnswersWithOid)
{
    npos::dev::msg::Open request("backup");
    request.sender = owner;

    store.handleRequest(request);
    store.handleRequest(request);

    CHECK_EQUAL(1, registers.enableCount);
    CHECK_EQUAL(2, api.responded.size());
    CHECK_EQUAL(npos::dev::msg::Open::SUCCESS, statusOfResponse());
    CHECK_EQUAL(2, api.responded[0].output.open.opened.serviceId);
}

TEST(BackupStore, OpenWithOtherNameIsIgnored)
{
    npos::dev::msg::Open request("other");

    store.handleRequest(request);

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(0, registers.enableCount);
}

TEST(BackupStore, RequestBeforeOpenIsRejected)
{
    auto request = read();

    store.handleRequest(request);

    CHECK_EQUAL(BackupRegisters::NOT_INITIALIZED, statusOfResponse());
}

TEST(BackupStore, RequestWithoutValuePointerIsRejected)
{
    open();
    BackupRegisters request(owner, BackupRegisters::READ, 0, nullptr);
    request.sender = owner;

    store.handleRequest(request);

    CHECK_EQUAL(BackupRegisters::INVALID_VALUE_PTR, statusOfResponse());
}

TEST(BackupStore, UnknownRegisterIsRejected)
{
    open();
    auto request = read(npos::dev::hal::BackupRegisters::COUNT);

    store.handleRequest(request);

    CHECK_EQUAL(BackupRegisters::INVALID_REGISTER, statusOfResponse());
}

TEST(BackupStore, ReadReturnsRegisterValue)
{
    open();
    registers.registers[5] = 0xBEEF;
    auto request = read(5);

    store.handleRequest(request);

    CHECK_EQUAL(BackupRegisters::SUCCESS, statusOfResponse());
    CHECK_EQUAL(0xBEEF, value);
}

TEST(BackupStore, WriteStoresValueInRegister)
{
    open();
    value = 0x1234;
    auto request = write(7);

    store.handleRequest(request);

    CHECK_EQUAL(BackupRegisters::SUCCESS, statusOfResponse());
    CHECK_EQUAL(0x1234, registers.registers[7]);
}

TEST(BackupStore, IgnoresOtherMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::dev::msg::Type::LED;

    store.handleRequest(message);

    CHECK_EQUAL(0, api.responded.size());
}

TEST(BackupStore, RejectsOpenFromDifferentOwnerWhenLocked)
{
    open();
    
    npos::dev::msg::Open request("backup");
    request.sender = { 3, 1 };  // different owner
    
    store.handleRequest(request);
    
    CHECK_EQUAL(npos::dev::msg::Open::LOCKED, statusOfResponse());
}

TEST(BackupStore, RejectsTransferFromDifferentOwner)
{
    open();
    
    auto request = read(0, { 3, 1 });  // different owner
    
    store.handleRequest(request);
    
    CHECK_EQUAL(BackupRegisters::LOCKED, statusOfResponse());
}
