#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::os::message
{
    enum Id : ipc::Message::Id
    {
        KERNEL = 0x0000,
        ERROR = 0x0001,

        DRIVER = 0x0200,
    };
}