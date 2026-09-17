#pragma once

/**
 * @file Status.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <cstdint>

namespace npos::os::filesystem
{
    enum Status : uint8_t
    {
        OK = 0x00,
        IO_ERROR,
        CORRUPTED,
        NO_DIRECTORY_ENTRY,
        ENTRY_ALREADY_EXISTS,
        NOT_A_DIRECOTRY,
        IS_A_DIRECTORY,
        DIRECTORY_NOT_EMPTY,
        BAD_FILE_NUMBER,
        FILE_TOO_LARGE,
        INVALID_PARAMETER,
        NO_SPACE_LEFT,
        NO_MEMORY_AVAILABLE,
        NO_DATA_AVAILABLE,
        DIRECTORY_READ_END
    };
}