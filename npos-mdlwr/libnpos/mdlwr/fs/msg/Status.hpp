#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::mdlwr::fs::msg
{
    enum Status : ipc::Message::Status
    {
        SUCCESS = 0,
        FILE_NOT_FOUND,
        DIR_NOT_FOUND,
        DEVICE_ERROR,
        BUFFER_TOO_SMALL,
        BUSY,
        ALREADY_EXISTS,
        NOT_A_FILE,
        NOT_A_DIRECTORY,
        NO_SPACE_LEFT,
        INVALID_PATH
    };
}