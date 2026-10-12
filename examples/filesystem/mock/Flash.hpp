#pragma once

/**
 * @file Flash.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <iostream>
#include <array>

#include <libnpos/fs/hal/Flash.hpp>

namespace mock
{
    class Flash : public npos::fs::hal::Flash
    {
    public:
        static constexpr size_t FLASH_SIZE = 512 * 1024;
        static constexpr size_t FLASH_BLOCK_SIZE = 8192;
        static constexpr size_t FLASH_PROGRAM_SIZE = 16;
        static constexpr size_t FLASH_BLOCK_CYCLES = 100000;
        std::array<uint8_t, FLASH_SIZE> storage{};
        bool locked = true;

        Flash()
        {
            std::fill(storage.begin(), storage.end(), 0xFF);
        }

        bool lock() override 
        {
            std::cout << "Emulator: Flash lock" << std::endl;
            locked = true;
            return true;
        }
        bool unlock() override 
        {
            std::cout << "Emulator: Flash unlock" << std::endl;
            locked = false;
            return true;
        }

        bool waitForLastOperation(Timeout) override 
        {
            std::cout << "Emulator: Flash waitForLastOperation" << std::endl;
            return true;
        }

        bool program(Address address, const uint8_t* buffer) override 
        {
            if(locked)
                return false;

            if(not buffer)
                return false;

            std::cout << "Emulator: Flash program, address: " << address << std::endl;
            std::copy(buffer, buffer + FLASH_PROGRAM_SIZE, std::next(storage.begin(), address));
            return true;
        }

        bool eraseBlocks(BlockNumber blockNumber, size_t numberOfBlocks) override 
        {
            std::cout << "Emulator: Flash eraseBlocks, blockNumber: " << blockNumber << ", numberOfBlocks: " << numberOfBlocks << std::endl;

            if(blockNumber + numberOfBlocks > getBlockCount())
                return false;

            for(size_t i = blockNumber; i < blockNumber + numberOfBlocks; ++i)
                std::fill_n(std::next(storage.begin(), i * FLASH_BLOCK_SIZE), FLASH_BLOCK_SIZE, 0xFF);

            return true;
        }

        bool read(Address address, uint8_t* buffer, size_t size) override 
        {
            std::cout << "Emulator: Flash read, address: " << address << ", size: " << size << std::endl;
            std::copy(std::next(storage.begin(), address), std::next(storage.begin(), address + size), buffer);
            return true;
        }

        Address getStartAddress() override { return 0; }
        size_t getProgramSize() override { return FLASH_PROGRAM_SIZE; }
        size_t getBlockCount() override { return FLASH_SIZE / FLASH_BLOCK_SIZE; }
        size_t getBlockSize() override { return FLASH_BLOCK_SIZE; }
        size_t getBlockCycles() override { return FLASH_BLOCK_CYCLES; }
    };
}