#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::mdlwr::dev::msg 
{
    enum Type : ipc::Message::Type
    {
        FILE_COPY = ipc::DRIVER_MESSAGE_TYPE,
        FILE_MOVE,
        FILE_READ,
        FILE_WRITE,
        DIR_MOVE,
        DIR_COPY,
        DIR_REMOVE
    };
}