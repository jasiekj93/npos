#pragma once

/**
 * @file FlagConverter.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <liblittlefs/lfs.h>

#include <libnpos/os/filesystem/message/File.hpp>

namespace npos::fs
{
    int osToLfsFlags(const os::filesystem::message::FileOpenRequest::Flags& flags)
    {
        int result = 0;

        for(auto& flag : flags)
        {
            switch (flag)
            {
                case os::filesystem::message::FileOpenRequest::Flag::READ_ONLY:
                    result |= LFS_O_RDONLY;
                    break;
                case os::filesystem::message::FileOpenRequest::Flag::WRITE_ONLY:
                    result |= LFS_O_WRONLY;
                    break;
                case os::filesystem::message::FileOpenRequest::Flag::READ_WRITE:
                    result |= LFS_O_RDWR;
                    break;
                case os::filesystem::message::FileOpenRequest::Flag::CREATE:
                    result |= LFS_O_CREAT;
                    break;
                case os::filesystem::message::FileOpenRequest::Flag::TRUNCATE:
                    result |= LFS_O_TRUNC;
                    break;
                case os::filesystem::message::FileOpenRequest::Flag::APPEND:
                    result |= LFS_O_APPEND;
                    break;
            }
        }

        return result;
    }
}