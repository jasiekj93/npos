#pragma once

/**
 * @file ServiceTask.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/os/Task.hpp>
#include <libnpos/os/filesystem/message/Directory.hpp>
#include <libnpos/os/filesystem/message/File.hpp>
#include <libnpos/os/filesystem/message/System.hpp>

namespace npos::os::filesystem
{
    using ServiceTask = os::ServiceTask<os::filesystem::message::FileOpenRequest,
                                        os::filesystem::message::FileCloseRequest,
                                        os::filesystem::message::FileReadRequest,
                                        os::filesystem::message::FileWriteRequest,
                                        os::filesystem::message::FileSyncRequest,
                                        os::filesystem::message::FileSeekRequest,
                                        os::filesystem::message::FileTruncateRequest,
                                        os::filesystem::message::FileTellRequest,
                                        os::filesystem::message::DirectoryMakeRequest,
                                        os::filesystem::message::DirectoryOpenRequest,
                                        os::filesystem::message::DirectoryCloseRequest,
                                        os::filesystem::message::DirectoryReadRequest,
                                        os::filesystem::message::DirectorySeekRequest,
                                        os::filesystem::message::DirectoryTellRequest,
                                        os::filesystem::message::SystemFormatRequest,
                                        os::filesystem::message::SystemMountRequest,
                                        os::filesystem::message::SystemUnmountRequest,
                                        os::filesystem::message::SystemRemoveRequest,
                                        os::filesystem::message::SystemRenameRequest,
                                        os::filesystem::message::SystemStatfsRequest>;
}