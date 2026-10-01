#pragma once

/**
 * @file Directory.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/vector.h>

#include <libnpos/os/filesystem/Info.hpp>
#include <libnpos/os/filesystem/Status.hpp>
#include <libnpos/os/filesystem/message/Id.hpp>

namespace npos::os::filesystem::message
{
    struct DirectoryMakeRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_MAKE_REQUEST;

        Path path;

        DirectoryMakeRequest() : os::Message(ID) {}
    };

    struct DirectoryMakeResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_MAKE_RESPONSE;

        Status status;

        DirectoryMakeResponse() : os::Message(ID) {}
    };

    struct DirectoryOpenRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_OPEN_REQUEST;

        Path path;

        DirectoryOpenRequest() : os::Message(ID) {}
    };

    struct DirectoryOpenResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_OPEN_RESPONSE;

        Status status;
        int directoryDescriptor;

        DirectoryOpenResponse() : os::Message(ID) {}
    };

    struct DirectoryCloseRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_CLOSE_REQUEST;

        int directoryDescriptor;

        DirectoryCloseRequest() : os::Message(ID) {}
    };

    struct DirectoryCloseResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_CLOSE_RESPONSE;

        Status status;

        DirectoryCloseResponse() : os::Message(ID) {}
    };

    struct DirectoryReadRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_READ_REQUEST;

        int directoryDescriptor;

        DirectoryReadRequest() : os::Message(ID) {}
    };

    struct DirectoryReadResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_READ_RESPONSE;

        Status status;
        Info info;

        DirectoryReadResponse() : os::Message(ID) {}
    };

    struct DirectorySeekRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_SEEK_REQUEST;

        int directoryDescriptor;
        int offset;

        DirectorySeekRequest() : os::Message(ID) {}
    };

    struct DirectorySeekResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_SEEK_RESPONSE;

        Status status;

        DirectorySeekResponse() : os::Message(ID) {}
    };

    struct DirectoryTellRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_TELL_REQUEST;

        int directoryDescriptor;

        DirectoryTellRequest() : os::Message(ID) {}
    };

    struct DirectoryTellResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::DIR_TELL_RESPONSE;

        Status status;
        int position;

        DirectoryTellResponse() : os::Message(ID) {}
    };
}
     