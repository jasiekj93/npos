#pragma once

/**
 * @file Log.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::cli::log
{
    enum Level : ipc::Message::Input::Attributes::Value
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
    };  

    struct Endl {};
    static Endl endl;
}