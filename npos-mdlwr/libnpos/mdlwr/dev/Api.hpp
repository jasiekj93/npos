#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/mdlwr/dev/msg/FileHash.hpp>

namespace npos::mdlwr::dev::api
{
    namespace file
    {
        inline void hash(ipc::Service& sender, etl::string_view path, etl::span<uint8_t> output)
        {
            npos::mdlwr::dev::msg::file::FileHash request(path, output);
            sender.getApi().send(sender, request);
        }
    }
}
