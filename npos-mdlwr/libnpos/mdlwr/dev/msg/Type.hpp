#pragma once

/**
 * @file Type.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <libnpos/mdlwr/fs/msg/Type.hpp>

namespace npos::mdlwr::dev::msg 
{
    enum Type : ipc::Message::Type
    {
        FILE_HASH = mdlwr::fs::msg::Type::UNUSED,

        UNUSED
    };
}