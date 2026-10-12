#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/fs/msg/file/Open.hpp>
#include <libnpos/fs/msg/file/Close.hpp>
#include <libnpos/fs/msg/file/Read.hpp>
#include <libnpos/fs/msg/file/Write.hpp>

namespace npos::fs::api
{
    void fileOpen(ipc::Service& sender, etl::string_view name, npos::fs::msg::file::Open::Flags flags)
    {
        npos::fs::msg::file::Open request(name, flags);
        sender.getApi().send(request);
    }

    void fileClose(ipc::Service& sender, ipc::Oid file)
    {
        npos::fs::msg::file::Close request(file);
        sender.getApi().send(request);
    }

    void fileRead(ipc::Service& sender, ipc::Oid file, ipc::Message::Input::ReadWrite::Length maxLength, etl::span<uint8_t> buffer)
    {
        npos::fs::msg::file::Read request(file, maxLength, buffer);
        sender.getApi().send(request);
    }

    void fileWrite(ipc::Service& sender, ipc::Oid file, etl::span<const uint8_t> buffer)
    {
        npos::fs::msg::file::Write request(file, buffer);
        sender.getApi().send(request);
    }
}