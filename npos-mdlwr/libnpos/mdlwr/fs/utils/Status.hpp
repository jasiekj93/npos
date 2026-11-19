#pragma once

/**
 * @file Status.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::utils
{
    // NO_DIR_ENTRY does not tell whether a file or a directory is missing, so the caller chooses `notFound`.
    inline msg::Status fromFsStatus(npos::fs::msg::Status status, msg::Status notFound)
    {
        switch(status)
        {
        case npos::fs::msg::Status::SUCCESS:
            return msg::Status::SUCCESS;
        case npos::fs::msg::Status::NO_DIR_ENTRY:
            return notFound;
        case npos::fs::msg::Status::ENTRY_EXISTS:
        case npos::fs::msg::Status::DIRECTORY_NOT_EMPTY:
            return msg::Status::ALREADY_EXISTS;
        case npos::fs::msg::Status::NOT_A_DIRECTORY:
            return msg::Status::NOT_A_DIRECTORY;
        case npos::fs::msg::Status::IS_A_DIRECTORY:
            return msg::Status::NOT_A_FILE;
        case npos::fs::msg::Status::NO_SPACE_LEFT:
        case npos::fs::msg::Status::FILE_TOO_LARGE:
            return msg::Status::NO_SPACE_LEFT;
        case npos::fs::msg::Status::NAME_TOO_LONG:
        case npos::fs::msg::Status::INVALID_PARAMETER:
            return msg::Status::INVALID_PATH;
        default:
            return msg::Status::DEVICE_ERROR;
        }
    }
}
