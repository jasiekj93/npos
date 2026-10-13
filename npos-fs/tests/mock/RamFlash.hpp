#pragma once

/**
 * @file RamFlash.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdint>
#include <cstring>

#include <libnpos/fs/hal/Flash.hpp>

namespace mock
{
    class RamFlash : public npos::fs::hal::Flash
    {
    public:
        static constexpr size_t BLOCK_SIZE = 4096;
        static constexpr size_t BLOCK_COUNT = 32;
        static constexpr size_t PROGRAM_SIZE = 16;

        RamFlash() { std::memset(storage, 0xFF, sizeof(storage)); }

        bool lock() override { locked = true; return true; }
        bool unlock() override { locked = false; return not failUnlock; }
        bool waitForLastOperation(Timeout) override { return not failWait; }

        bool program(Address address, const uint8_t* buffer) override
        {
            if(locked or failProgram)
                return false;

            std::memcpy(storage + address, buffer, PROGRAM_SIZE);
            programCount++;
            return true;
        }

        bool eraseBlocks(BlockNumber block, size_t count) override
        {
            if(failErase or block + count > BLOCK_COUNT)
                return false;

            std::memset(storage + block * BLOCK_SIZE, 0xFF, count * BLOCK_SIZE);
            return true;
        }

        bool read(Address address, uint8_t* buffer, size_t size) override
        {
            std::memcpy(buffer, storage + address, size);
            return true;
        }

        Address getStartAddress() override { return 0; }
        size_t getProgramSize() override { return PROGRAM_SIZE; }
        size_t getBlockCount() override { return BLOCK_COUNT; }
        size_t getBlockSize() override { return BLOCK_SIZE; }
        size_t getBlockCycles() override { return 1000; }

        bool locked = true;
        bool failProgram = false;
        bool failErase = false;
        bool failUnlock = false;
        bool failWait = false;
        int programCount = 0;
        uint8_t storage[BLOCK_SIZE * BLOCK_COUNT];
    };
}
