#pragma once

/**
 * @file Type.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::dev::msg
{
    enum Type : ipc::Message::Type
    {
        OPEN = ipc::DEVICE_MESSAGE_TYPE,
        CLOCK,
        LED,
    };
}