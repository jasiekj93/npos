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
        CLOCK = kernel::DEVICE_MESSAGE_TYPE;
    };
}