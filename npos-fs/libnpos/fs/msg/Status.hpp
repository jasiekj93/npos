#pragma once

/**
 * @file Status.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg
{
    enum Status : ipc::Message::Status
    {
        SUCCESS = 0,    // No error
        IO_ERROR,
        CORRUPTED,
        NO_DIR_ENTRY,
        ENTRY_EXISTS,
        NOT_A_DIRECTORY,
        IS_A_DIRECTORY,
        DIRECTORY_NOT_EMPTY,
        BAD_FILE_NUMBER,
        FILE_TOO_LARGE,
        INVALID_PARAMETER,
        NO_SPACE_LEFT,
        NO_MEMORY,
        NO_ATTRIBUTE,
        NAME_TOO_LONG,
        UNKNOWN_ERROR,
    };
}