#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::os::message
{
    enum Id : ipc::Message::Id
    {
        KERNEL = 0x0000,
        DRIVER = 0x0200,
    };
}