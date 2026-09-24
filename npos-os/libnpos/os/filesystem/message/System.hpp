#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/os/filesystem/Status.hpp>
#include <libnpos/os/filesystem/Info.hpp>
#include <libnpos/os/filesystem/message/Id.hpp>

namespace npos::os::filesystem::message
{
    struct SystemFormatRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_FORMAT_REQUEST;

        SystemFormatRequest() : os::Message(ID) {}
    };

    struct SystemFormatResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_FORMAT_RESPONSE;

        Status status;

        SystemFormatResponse() : os::Message(ID) {}
    };

    struct SystemMountRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_MOUNT_REQUEST;

        SystemMountRequest() : os::Message(ID) {}
    };

    struct SystemMountResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_MOUNT_RESPONSE;

        Status status;

        SystemMountResponse() : os::Message(ID) {}
    };

    struct SystemUnmountRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_UNMOUNT_REQUEST;

        SystemUnmountRequest() : os::Message(ID) {}
    };

    struct SystemUnmountResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_UNMOUNT_RESPONSE;

        Status status;

        SystemUnmountResponse() : os::Message(ID) {}
    };

    struct SystemRemoveRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_REMOVE_REQUEST;

        Path path;

        SystemRemoveRequest() : os::Message(ID) {}
    };

    struct SystemRemoveResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_REMOVE_RESPONSE;

        Status status;

        SystemRemoveResponse() : os::Message(ID) {}
    };

    struct SystemRenameRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_RENAME_REQUEST;

        Path oldPath;
        Path newPath;

        SystemRenameRequest() : os::Message(ID) {}
    };

    struct SystemRenameResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_RENAME_RESPONSE;

        Status status;

        SystemRenameResponse() : os::Message(ID) {}
    };

    struct SystemStatfsRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_STATFS_REQUEST;

        Path path;

        SystemStatfsRequest() : os::Message(ID) {}
    };

    struct SystemStatfsResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::SYSTEM_STATFS_RESPONSE;

        Status status;
        Info info;

        SystemStatfsResponse() : os::Message(ID) {}
    };
}