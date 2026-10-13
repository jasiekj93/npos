/**
 * @file BackupRegisters.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/BackupRegisters.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorBackupRegisters)
{
    npos::emulator::BackupRegisters registers;
};

TEST(EmulatorBackupRegisters, AccessCanBeToggled)
{
    registers.enableAccess();
    CHECK_TRUE(registers.accessEnabled);

    registers.disableAccess();
    CHECK_FALSE(registers.accessEnabled);
}

TEST(EmulatorBackupRegisters, KeepsWrittenValues)
{
    registers.write(3, 0xCAFE);

    CHECK_EQUAL(0xCAFE, registers.read(3));
    CHECK_EQUAL(0, registers.read(4));
}

TEST(EmulatorBackupRegisters, OutOfRangeIndexIsIgnored)
{
    registers.write(npos::dev::hal::BackupRegisters::COUNT, 1);

    CHECK_EQUAL(0, registers.read(npos::dev::hal::BackupRegisters::COUNT));
}
