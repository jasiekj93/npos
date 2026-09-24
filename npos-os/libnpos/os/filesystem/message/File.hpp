#pragma once

/**
 * @file File.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/vector.h>
#include <etl/span.h>

#include <libnpos/os/filesystem/Path.hpp>
#include <libnpos/os/filesystem/Status.hpp>
#include <libnpos/os/filesystem/message/Id.hpp>

namespace npos::os::filesystem::message
{
    struct FileOpenRequest : public os::Message
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

        FileOpenRequest() : os::Message(ID) {}
    };

    struct FileOpenResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_OPEN_RESPONSE;

        Status status;
        size_t fileSize;
        int fileDescriptor;

        FileOpenResponse() : os::Message(ID) {}
    };

    struct FileCloseRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_CLOSE_REQUEST;

        int fileDescriptor;

        FileCloseRequest() : os::Message(ID) {}
    };

    struct FileCloseResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_CLOSE_RESPONSE;

        Status status;

        FileCloseResponse() : os::Message(ID) {}
    };

    struct FileWriteRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_WRITE_REQUEST;

        int fileDescriptor;
        etl::span<const uint8_t> data;

        FileWriteRequest() : os::Message(ID) {}
    };

    struct FileWriteResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_WRITE_RESPONSE;

        Status status;
        size_t bytesWritten;

        FileWriteResponse() : os::Message(ID) {}
    };

    struct FileReadRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_READ_REQUEST;

        int fileDescriptor;
        etl::span<uint8_t> buffer;

        FileReadRequest() : os::Message(ID) {}
    };

    struct FileReadResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_READ_RESPONSE;

        Status status;
        size_t bytesRead;

        FileReadResponse() : os::Message(ID) {}
    };

    struct FileSyncRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SYNC_REQUEST;

        int fileDescriptor;

        FileSyncRequest() : os::Message(ID) {}
    };

    struct FileSyncResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SYNC_RESPONSE;

        Status status;

        FileSyncResponse() : os::Message(ID) {}
    };

    struct FileSeekRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SEEK_REQUEST;

        int fileDescriptor;
        size_t offset;
        int whence;

        FileSeekRequest() : os::Message(ID) {}
    };

    struct FileSeekResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_SEEK_RESPONSE;

        Status status;
        size_t fileOffset;

        FileSeekResponse() : os::Message(ID) {}
    };
    

    struct FileTellRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TELL_REQUEST;

        int fileDescriptor;

        FileTellRequest() : os::Message(ID) {}
    };

    struct FileTellResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TELL_RESPONSE;

        Status status;
        size_t fileOffset;

        FileTellResponse() : os::Message(ID) {}
    };

    struct FileTruncateRequest : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TRUNCATE_REQUEST;

        int fileDescriptor;
        size_t size;

        FileTruncateRequest() : os::Message(ID) {}
    };

    struct FileTruncateResponse : public os::Message
    {
        static constexpr auto ID = filesystem::message::Id::FILE_TRUNCATE_RESPONSE;

        Status status;

        FileTruncateResponse() : os::Message(ID) {}
    };
}