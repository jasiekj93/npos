#pragma once

/**
 * @file Api.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/mdlwr/fs/msg/file/Copy.hpp>
#include <libnpos/mdlwr/fs/msg/file/Move.hpp>
#include <libnpos/mdlwr/fs/msg/file/Read.hpp>
#include <libnpos/mdlwr/fs/msg/file/Write.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Copy.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Move.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Remove.hpp>

namespace npos::mdlwr::fs::api
{
    namespace file
    {
        inline void copy(ipc::Service& sender, etl::string_view oldPath, etl::string_view newPath)
        {
            npos::mdlwr::fs::msg::file::Copy request(oldPath, newPath);
            sender.getApi().send(sender, request);
        }

        inline void move(ipc::Service& sender, etl::string_view oldPath, etl::string_view newPath)
        {
            npos::mdlwr::fs::msg::file::Move request(oldPath, newPath);
            sender.getApi().send(sender, request);
        }

        inline void read(ipc::Service& sender, etl::string_view pathToFile, etl::span<uint8_t> output)
        {
            npos::mdlwr::fs::msg::file::Read request(pathToFile, output);
            sender.getApi().send(sender, request);
        }

        inline void write(ipc::Service& sender, etl::string_view pathToFile, etl::span<const uint8_t> input, bool append)
        {
            npos::mdlwr::fs::msg::file::Write request(pathToFile, input, append);
            sender.getApi().send(sender, request);
        }
    }

    namespace dir
    {
        inline void copy(ipc::Service& sender, etl::string_view oldPath, etl::string_view newPath)
        {
            npos::mdlwr::fs::msg::dir::Copy request(oldPath, newPath);
            sender.getApi().send(sender, request);
        }

        inline void move(ipc::Service& sender, etl::string_view oldPath, etl::string_view newPath)
        {
            npos::mdlwr::fs::msg::dir::Move request(oldPath, newPath);
            sender.getApi().send(sender, request);
        }

        inline void remove(ipc::Service& sender, etl::string_view path)
        {
            npos::mdlwr::fs::msg::dir::Remove request(path);
            sender.getApi().send(sender, request);
        }
    }
}
