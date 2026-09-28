#pragma once

/**
 * @file Type.hpp
 * @author Adrian Szczepanski
 * @date 27-09-2026
 */

#include <libnpos/kernel/Message.hpp>

namespace npos::device
{
    enum Type : kernel::Message::Type
    {
        ERROR = kernel::DEVICE_MESSAGE_TYPE,
        CLOCK,
        LED,
        TRANSCEIVER_OPEN,
        TRANSCEIVER_CLOSE,
        TRANSCEIVER_READ,
        TRANSCEIVER_WRITE,
    };
}