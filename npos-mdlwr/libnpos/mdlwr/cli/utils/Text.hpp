#pragma once

/**
 * @file Text.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/mdlwr/dev/msg/FileHash.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::cli::utils::text
{
    inline const char* describe(mdlwr::fs::msg::Status status)
    {
        // Nobody serves the request: the bus or the service task answers with its own status.
        if(status == npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED or
            status == npos::ipc::Message::STATUS_PORT_NOT_FOUND or
            status == npos::ipc::Message::STATUS_SERVICE_NOT_FOUND)
        {
            return "service not available";
        }

        switch(status)
        {
        case mdlwr::fs::msg::Status::SUCCESS: return "success";
        case mdlwr::fs::msg::Status::FILE_NOT_FOUND: return "no such file";
        case mdlwr::fs::msg::Status::DIR_NOT_FOUND: return "no such directory";
        case mdlwr::fs::msg::Status::DEVICE_ERROR: return "I/O error";
        case mdlwr::fs::msg::Status::BUFFER_TOO_SMALL: return "buffer too small";
        case mdlwr::fs::msg::Status::BUSY: return "service busy";
        case mdlwr::fs::msg::Status::ALREADY_EXISTS: return "file exists";
        case mdlwr::fs::msg::Status::NOT_A_FILE: return "is a directory";
        case mdlwr::fs::msg::Status::NOT_A_DIRECTORY: return "not a directory";
        case mdlwr::fs::msg::Status::NO_SPACE_LEFT: return "no space left";
        case mdlwr::fs::msg::Status::INVALID_PATH: return "invalid path";
        default: return "unknown error";
        }
    }

    inline const char* describe(mdlwr::dev::msg::FileHash::Status status)
    {
        using Status = mdlwr::dev::msg::FileHash;

        // Nobody serves the request: the bus or the service task answers with its own status.
        if(status == npos::ipc::Message::STATUS_MESSAGE_NOT_SUPPORTED or
            status == npos::ipc::Message::STATUS_PORT_NOT_FOUND or
            status == npos::ipc::Message::STATUS_SERVICE_NOT_FOUND)
        {
            return "service not available";
        }

        switch(status)
        {
        case Status::SUCCESS: return "success";
        case Status::DEVICE_ERROR: return "I/O error";
        case Status::BUFFER_TOO_SMALL: return "buffer too small";
        case Status::HASH_NOT_FOUND: return "hash device not found";
        case Status::HASH_BUSY: return "hash device busy";
        case Status::FILE_NOT_FOUND: return "no such file";
        case Status::IS_A_DIRECTORY: return "is a directory";
        case Status::INVALID_PATH: return "invalid path";
        case Status::BUSY: return "service busy";
        default: return "unknown error";
        }
    }
}
