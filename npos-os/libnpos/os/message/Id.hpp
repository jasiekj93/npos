#pragma once

#include <libnpos/ipc/Message.hpp>

namespace npos::os::message
{
    enum Id : ipc::Message::Id
    {
        SYSTEM_ERROR = 0x0000,
        SYSLOG = 0x0001,

        DEVICE = 0x0200,
        DRIVER = 0x0300,
        FILESYSTEM = 0x0400,
        CLI = 0x0500,
        USER = 0x1000
    };
}