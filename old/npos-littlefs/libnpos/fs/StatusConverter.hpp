#pragma once

/**
 * @file StatusConverter.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <liblittlefs/lfs.h>

#include <libnpos/os/filesystem/Status.hpp>

namespace npos::fs
{
    os::filesystem::Status lfsToOsStatus(int lfsStatus)
    {
        switch (lfsStatus)
        {
        case LFS_ERR_OK:          
            return os::filesystem::Status::OK;
        case LFS_ERR_IO:           
            return os::filesystem::Status::IO_ERROR;
        case LFS_ERR_CORRUPT:     
            return os::filesystem::Status::CORRUPTED;
        case LFS_ERR_NOENT:       
            return os::filesystem::Status::NO_DIRECTORY_ENTRY;
        case LFS_ERR_EXIST:       
            return os::filesystem::Status::ALREADY_EXISTS;
        case LFS_ERR_NOTDIR:      
            return os::filesystem::Status::NOT_A_DIRECTORY;
        case LFS_ERR_ISDIR:       
            return os::filesystem::Status::IS_A_DIRECTORY;
        case LFS_ERR_NOTEMPTY:    
            return os::filesystem::Status::DIRECTORY_NOT_EMPTY;
        case LFS_ERR_BADF:        
            return os::filesystem::Status::BAD_FILE_NUMBER;
        case LFS_ERR_FBIG:        
            return os::filesystem::Status::FILE_TOO_LARGE;
        case LFS_ERR_INVAL:       
            return os::filesystem::Status::INVALID_PARAMETER;
        case LFS_ERR_NOSPC:       
            return os::filesystem::Status::NO_SPACE_LEFT;
        case LFS_ERR_NOMEM:       
            return os::filesystem::Status::NO_MEMORY_AVAILABLE;
        case LFS_ERR_NOATTR:      
            return os::filesystem::Status::NO_DATA_AVAILABLE;
        case LFS_ERR_NAMETOOLONG: 
            return os::filesystem::Status::INVALID_PARAMETER;
        default:
            return os::filesystem::Status::IO_ERROR;
        }
    }
}