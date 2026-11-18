#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::mdlwr::dev::msg
{
    enum Status : ipc::Message::Status
    {
        SUCCESS = 0,
        FILE_NOT_FOUND,
        DIR_NOT_FOUND,
        DEVICE_ERROR,
        BUFFER_TOO_SMALL,
        BUSY
    };
}