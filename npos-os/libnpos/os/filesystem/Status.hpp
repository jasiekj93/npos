#pragma once

/**
 * @file Status.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <libnpos/os/Message.hpp>

namespace npos::os::filesystem
{
    enum Status : os::Message::Status
    {
        OK = 0x00,
        IO_ERROR,
        CORRUPTED,
        NO_DIRECTORY_ENTRY,
        ALREADY_EXISTS,
        NOT_A_DIRECTORY,
        IS_A_DIRECTORY,
        DIRECTORY_NOT_EMPTY,
        BAD_FILE_NUMBER,
        FILE_TOO_LARGE,
        INVALID_PARAMETER,
        NO_SPACE_LEFT,
        NO_MEMORY_AVAILABLE,
        NO_DATA_AVAILABLE,
        DIRECTORY_READ_END,
        TOO_MANY_FILES_OPENED,
        FILE_NOT_FOUND,
        DIRECTORY_NOT_FOUND
    };
}