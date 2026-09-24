#pragma once

#include <liblittlefs/lfs.h>

#include <libnpos/os/filesystem/Info.hpp>

namespace npos::fs
{
    inline os::filesystem::Info lfsToOsInfo(const lfs_info& info)
    {
        os::filesystem::Info result;
        result.name = info.name;
        result.size = info.size;

        if(info.type == LFS_TYPE_DIR)
            result.type = os::filesystem::Info::Type::DIRECTORY;
        else if(info.type == LFS_TYPE_REG)
            result.type = os::filesystem::Info::Type::FILE;

        return result;
    }
}