#pragma once

/**
 * @file BackupRegisters.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdint>
#include <iostream>

#include <libnpos/dev/hal/BackupRegisters.hpp>

namespace npos::emulator
{
    class BackupRegisters : public npos::dev::hal::BackupRegisters
    {
    public:
        explicit BackupRegisters(bool verbose = false) : verbose(verbose) {}

        void enableAccess() override { accessEnabled = true; }
        void disableAccess() override { accessEnabled = false; }

        void write(uint32_t index, uint32_t value) override
        {
            if(index >= COUNT)
                return;

            registers[index] = value;

            if(verbose)
                std::cout << "Emulator: BackupRegisters write " << index << " = " << value << std::endl;
        }

        uint32_t read(uint32_t index) override
        {
            return index < COUNT ? registers[index] : 0;
        }

        bool accessEnabled = false;
        bool verbose;
        uint32_t registers[COUNT] = {};
    };
}
