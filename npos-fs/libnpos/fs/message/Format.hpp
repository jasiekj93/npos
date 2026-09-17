#pragma once

/**
 * @file Format.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <libnpos/fs/Path.hpp>
#include <libnpos/fs/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::fs::message
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
    };

    struct OpenRequest
    {
        enum Flags : uint8_t
        {
            READ_ONLY = 0,
            WRITE_ONLY,
            READ_WRITE,
            CREATE,
            EXCLUSIVE,
            TRUNCATE,
            APPEND,
        };

        Path path;
    };
}