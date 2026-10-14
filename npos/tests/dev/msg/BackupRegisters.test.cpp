/**
 * @file BackupRegisters.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/msg/BackupRegisters.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(BackupRegisters)
{
    npos::dev::msg::BackupRegisters::Value value = 0;
};

TEST(BackupRegisters, StoresOperationRegisterAndValuePointer)
{
    npos::dev::msg::BackupRegisters request({ 1, 2 }, npos::dev::msg::BackupRegisters::WRITE, 7, &value);

    CHECK_EQUAL(npos::dev::msg::Type::BACKUP_REGISTERS, request.type);
    CHECK_EQUAL(npos::dev::msg::BackupRegisters::WRITE, request.getOperation());
    CHECK_EQUAL(7, request.getRegister());
    POINTERS_EQUAL(&value, request.getValue());
}

TEST(BackupRegisters, SetValueWritesThroughPointer)
{
    npos::dev::msg::BackupRegisters request({ 1, 2 }, npos::dev::msg::BackupRegisters::READ, 0, &value);

    request.setValue(0xCAFE);

    CHECK_EQUAL(0xCAFE, value);
}

TEST(BackupRegisters, SetValueWithoutPointerIsIgnored)
{
    npos::dev::msg::BackupRegisters request({ 1, 2 }, npos::dev::msg::BackupRegisters::READ, 0, nullptr);

    request.setValue(1);

    POINTERS_EQUAL(nullptr, request.getValue());
}

TEST(BackupRegisters, ResponseCarriesStatus)
{
    npos::dev::msg::BackupRegisters request({ 1, 2 }, npos::dev::msg::BackupRegisters::READ, 0, &value);

    request.setStatus(npos::dev::msg::BackupRegisters::INVALID_REGISTER);

    CHECK_EQUAL(npos::dev::msg::BackupRegisters::INVALID_REGISTER, request.getStatus());
}
