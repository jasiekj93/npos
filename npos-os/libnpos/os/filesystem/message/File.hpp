#pragma once

/**
 * @file Format.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/vector.h>

#include <libnpos/os/filesystem/Path.hpp>
#include <libnpos/os/filesystem/Status.hpp>
#include <libnpos/os/filesystem/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::filesystem::message
{
    struct FileOpenRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_OPEN_REQUEST;

        enum Flag : uint8_t
        {
            READ_ONLY = 0,
            WRITE_ONLY,
            READ_WRITE,
            CREATE,
            EXCLUSIVE,
            TRUNCATE,
            APPEND,
        };

        static constexpr size_t FLAG_COUNT = 7;
        using Flags = etl::vector<Flag, FLAG_COUNT>;

        Path path;
        Flags flags;

        FileOpenRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileOpenResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_OPEN_RESPONSE;

        Status status;
        size_t fileSize;
        int fileDescriptor;

        FileOpenResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileCloseRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_CLOSE_REQUEST;

        int fileDescriptor;

        FileCloseRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileCloseResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_CLOSE_RESPONSE;

        Status status;

        FileCloseResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileWriteRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_WRITE_REQUEST;

        int fileDescriptor;
        etl::span<const uint8_t> data;

        FileWriteRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileWriteResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_WRITE_RESPONSE;

        Status status;
        size_t bytesWritten;

        FileWriteResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileReadRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_READ_REQUEST;

        int fileDescriptor;
        etl::span<uint8_t> buffer;

        FileReadRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileReadResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_READ_RESPONSE;

        Status status;
        size_t bytesRead;

        FileReadResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileSyncRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SYNC_REQUEST;

        int fileDescriptor;

        FileSyncRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileSyncResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SYNC_RESPONSE;

        Status status;

        FileSyncResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileSeekRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SEEK_REQUEST;

        int fileDescriptor;
        size_t offset;
        int whence;

        FileSeekRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileSeekResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SEEK_RESPONSE;

        Status status;
        size_t fileOffset;

        FileSeekResponse()
            : os::message::Response(ID)
        {
        }
    };
    

    struct FileTellRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TELL_REQUEST;

        int fileDescriptor;

        FileTellRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileTellResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TELL_RESPONSE;

        Status status;
        size_t fileOffset;

        FileTellResponse()
            : os::message::Response(ID)
        {
        }
    };

    struct FileTruncateRequest : public os::message::Request
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TRUNCATE_REQUEST;

        int fileDescriptor;
        size_t size;

        FileTruncateRequest(Request::SenderId senderId)
            : os::message::Request(ID, senderId)
        {
        }
    };

    struct FileTruncateResponse : public os::message::Response
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TRUNCATE_RESPONSE;

        Status status;

        FileTruncateResponse()
            : os::message::Response(ID)
        {
        }
    };
}