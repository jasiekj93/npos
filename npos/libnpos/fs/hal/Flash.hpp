#pragma once

/**
 * @file Flash.hpp
 * @author Adrian Szczepanski
 * @date 02-09-2026
 */

#include <cstdint>
#include <cstddef>

namespace npos::fs::hal
{
    class Flash
    {
    public:
        using Timeout = uint32_t;
        using Address = uint32_t;
        using BlockNumber = uint32_t;

        virtual ~Flash() = default;

        virtual bool lock() = 0;
        virtual bool unlock() = 0;

        virtual bool waitForLastOperation(Timeout) = 0;
        virtual bool program(Address address, const uint8_t* buffer) = 0;
        virtual bool eraseBlocks(BlockNumber, size_t numberOfBlocks) = 0;
        virtual bool read(Address address, uint8_t* buffer, size_t size) = 0;

        virtual Address getStartAddress() = 0;
        virtual size_t getProgramSize() = 0;
        virtual size_t getBlockCount() = 0;
        virtual size_t getBlockSize() = 0;
        virtual size_t getBlockCycles() = 0;
    };
}