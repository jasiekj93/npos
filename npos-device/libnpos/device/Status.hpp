#pragma once

#include <libnpos/kernel/Message.hpp>

namespace npos::device
{
    enum Status : kernel::Message::Status
    {
        OK = 0,
        DEVICE_ERROR,
        DEVICE_BUSY,
        INVALID_PARAMETER,
        INVALID_OPERATION
    };
}