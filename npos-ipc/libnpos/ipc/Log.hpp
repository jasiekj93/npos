#pragma once

/**
 * @file Log.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/string.h>
#include <etl/string_stream.h>

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc::log
{
    enum Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
    };  

    class Stream
    {
    public:
        Stream();
    };
}