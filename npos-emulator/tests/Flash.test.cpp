/**
 * @file Flash.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/emulator/Flash.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(EmulatorFlash)
{
    npos::emulator::Flash flash;
    uint8_t page[npos::emulator::Flash::FLASH_PROGRAM_SIZE];

    void setup() override
    {
        for(auto& b : page)
            b = 0xA5;
    }
};

TEST(EmulatorFlash, GeometryIsConsistent)
{
    CHECK_EQUAL(0, flash.getStartAddress());
    CHECK_EQUAL(npos::emulator::Flash::FLASH_BLOCK_SIZE, flash.getBlockSize());
    CHECK_EQUAL(npos::emulator::Flash::FLASH_PROGRAM_SIZE, flash.getProgramSize());
    CHECK_EQUAL(npos::emulator::Flash::FLASH_SIZE / npos::emulator::Flash::FLASH_BLOCK_SIZE, flash.getBlockCount());
    CHECK_EQUAL(npos::emulator::Flash::FLASH_BLOCK_CYCLES, flash.getBlockCycles());
}

TEST(EmulatorFlash, StartsErased)
{
    uint8_t buffer[4];

    flash.read(100, buffer, sizeof(buffer));

    for(auto b : buffer)
        CHECK_EQUAL(0xFF, b);
}

TEST(EmulatorFlash, ProgrammingRequiresUnlock)
{
    CHECK_FALSE(flash.program(0, page));

    CHECK_TRUE(flash.unlock());
    CHECK_TRUE(flash.program(0, page));
    CHECK_TRUE(flash.lock());
    CHECK_FALSE(flash.program(16, page));
}

TEST(EmulatorFlash, ProgrammedDataCanBeRead)
{
    uint8_t buffer[npos::emulator::Flash::FLASH_PROGRAM_SIZE];
    flash.unlock();

    flash.program(32, page);
    flash.read(32, buffer, sizeof(buffer));

    MEMCMP_EQUAL(page, buffer, sizeof(buffer));
}

TEST(EmulatorFlash, ProgrammingNullBufferFails)
{
    flash.unlock();

    CHECK_FALSE(flash.program(0, nullptr));
}

TEST(EmulatorFlash, EraseRestoresBlockContent)
{
    uint8_t buffer[4];
    flash.unlock();
    flash.program(0, page);

    CHECK_TRUE(flash.eraseBlocks(0, 1));
    flash.read(0, buffer, sizeof(buffer));

    CHECK_EQUAL(0xFF, buffer[0]);
}

TEST(EmulatorFlash, EraseBeyondFlashFails)
{
    CHECK_FALSE(flash.eraseBlocks(flash.getBlockCount(), 1));
    CHECK_FALSE(flash.eraseBlocks(flash.getBlockCount() - 1, 2));
}

TEST(EmulatorFlash, WaitForLastOperationSucceeds)
{
    CHECK_TRUE(flash.waitForLastOperation(100));
}

TEST(EmulatorFlash, VerboseModeDoesNotChangeBehaviour)
{
    npos::emulator::Flash verbose(true);

    CHECK_TRUE(verbose.unlock());
    CHECK_TRUE(verbose.waitForLastOperation(1));
    CHECK_TRUE(verbose.program(0, page));
    CHECK_TRUE(verbose.eraseBlocks(0, 1));
    CHECK_TRUE(verbose.read(0, page, 1));
    CHECK_TRUE(verbose.lock());
}
