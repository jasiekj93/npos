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
#include <libnpos/fs/msg/dir/Open.hpp>
#include <libnpos/fs/msg/dir/Close.hpp>
#include <libnpos/fs/msg/dir/Read.hpp>

namespace npos::fs::api
{
    namespace file
    {
        inline void open(ipc::Service& sender, etl::string_view name, ipc::Message::Input::Open::Flags flags)
        {
            npos::fs::msg::file::Open request(name, flags);
            sender.getApi().send(request);
        }

        inline void close(ipc::Service& sender, ipc::Oid file)
        {
            npos::fs::msg::file::Close request(file);
            sender.getApi().send(request);
        }

        inline void read(ipc::Service& sender, ipc::Oid file, ipc::Message::Input::ReadWrite::Length maxLength, etl::span<uint8_t> buffer)
        {
            npos::fs::msg::file::Read request(file, maxLength, buffer);
            sender.getApi().send(request);
        }

        inline void write(ipc::Service& sender, ipc::Oid file, etl::span<const uint8_t> buffer)
        {
            npos::fs::msg::file::Write request(file, buffer);
            sender.getApi().send(request);
        }
    }

    namespace dir
    {
        inline void open(ipc::Service& sender, etl::string_view path)
        {
            npos::fs::msg::dir::Open request(path);
            sender.getApi().send(request);
        }

        inline void close(ipc::Service& sender, ipc::Oid dir)
        {
            npos::fs::msg::dir::Close request(dir);
            sender.getApi().send(request);
        }

        inline void read(ipc::Service& sender, ipc::Oid dir, npos::fs::msg::dir::Entry& entry)
        {
            npos::fs::msg::dir::Read request(dir, entry);
            sender.getApi().send(request);
        }
    }
}
