#pragma once

/**
 * @file FakeBackupRegisters.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/dev/hal/BackupRegisters.hpp>

namespace mock
{
    class FakeBackupRegisters : public npos::dev::hal::BackupRegisters
    {
    public:
        void enableAccess() override { enableCount++; }
        void disableAccess() override {}
        void write(uint32_t index, uint32_t value) override { registers[index] = value; }
        uint32_t read(uint32_t index) override { return registers[index]; }

        int enableCount = 0;
        uint32_t registers[COUNT] = {};
    };
}
