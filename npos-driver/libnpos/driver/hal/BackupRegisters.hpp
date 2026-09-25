#pragma once

/**
 * @file BackupRegisters.hpp
 * @author Adrian Szczepanski
 * @date 04-09-2026
 */

#include <cstdint>

namespace npos::driver::hal
{
    class BackupRegisters
    {
    public:
        static constexpr uint32_t COUNT = 32;

        virtual ~BackupRegisters() = default;

        virtual void enableAccess() = 0;
        virtual void disableAccess() = 0;

        virtual void write(uint32_t index, uint32_t value) = 0;
        virtual uint32_t read(uint32_t index) = 0;
    };
}