#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 14-10-2026
 */

#include <libnpos/cli/utils/LogStream.hpp>
#include <libnpos/cli/msg/RegisterName.hpp>

namespace npos::cli::api
{
    inline npos::ipc::Oid recipient = npos::ipc::NULL_OID;

    inline utils::LogStream syslog(ipc::Service& sender, log::Level level)
    {
        return utils::LogStream(sender, recipient, level);
    }

    inline void registerName(ipc::Service& sender, etl::string_view name)
    {
        msg::RegisterName request(recipient, name);
        sender.getApi().send(sender, request);
    }
}