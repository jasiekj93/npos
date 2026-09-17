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
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::filesystem::message
{
    struct DirectoryMakeRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_MAKE_REQUEST;

        Path path;

        DirectoryMakeRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectoryMakeResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_MAKE_RESPONSE;

        Status status;

        DirectoryMakeResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };

    struct DirectoryOpenRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_OPEN_REQUEST;

        Path path;

        DirectoryOpenRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectoryOpenResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_OPEN_RESPONSE;

        Status status;
        int directoryDescriptor;

        DirectoryOpenResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };

    struct DirectoryCloseRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_CLOSE_REQUEST;

        int directoryDescriptor;

        DirectoryCloseRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectoryCloseResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_CLOSE_RESPONSE;

        Status status;

        DirectoryCloseResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };

    struct DirectoryReadRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_READ_REQUEST;

        int directoryDescriptor;

        DirectoryReadRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectoryReadResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_READ_RESPONSE;

        Status status;
        Info info;

        DirectoryReadResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };

    struct DirectorySeekRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_SEEK_REQUEST;

        int directoryDescriptor;
        int offset;

        DirectorySeekRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectorySeekResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_SEEK_RESPONSE;

        Status status;

        DirectorySeekResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };

    struct DirectoryTellRequest : public os::message::Request
    {
        static constexpr auto ID = Id::DIR_TELL_REQUEST;

        int directoryDescriptor;

        DirectoryTellRequest(Request::SenderId senderId)
            : Request(senderId)
        {
        }
    };

    struct DirectoryTellResponse : public os::message::Response
    {
        static constexpr auto ID = Id::DIR_TELL_RESPONSE;

        Status status;
        int position;

        DirectoryTellResponse(Request::SenderId senderId)
            : Response(senderId)
        {
        }
    };
}
     