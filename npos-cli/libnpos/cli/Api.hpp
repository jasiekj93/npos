#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <libnpos/cli/utils/LogStream.hpp>

namespace npos::cli::api
{
    utils::LogStream syslog(ipc::Service& sender, log::Level level)
    {
        return utils::LogStream(sender, level);
    }
}