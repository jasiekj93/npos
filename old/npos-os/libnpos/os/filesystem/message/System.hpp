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
    struct SystemFormat : public os::Message
    {
        SystemFormat()
        {
            this->type = filesystem::message::Id::SYSTEM_FORMAT;
        }

        void setStatus(filesystem::Status status)
        {
            this->status = status;
        }

        inline filesystem::Status getStatus() const
        {
            return static_cast<filesystem::Status>(this->status);
        }
    };

    struct SystemMount : public os::Message
    {
        SystemMount()
        {
            this->type = filesystem::message::Id::SYSTEM_MOUNT;
        }

        void setStatus(filesystem::Status status)
        {
            this->status = status;
        }

        inline filesystem::Status getStatus() const
        {
            return static_cast<filesystem::Status>(this->status);
        }
    };

    struct SystemUnmount : public os::Message
    {
        SystemUnmount() 
        {
            this->type = filesystem::message::Id::SYSTEM_UNMOUNT;
        }

        void setStatus(filesystem::Status status)
        {
            this->status = status;
        }

        inline filesystem::Status getStatus() const
        {
            return static_cast<filesystem::Status>(this->status);
        }
    };

    struct SystemRemove : public os::Message
    {
        SystemRemove()
        {
            this->type = filesystem::message::Id::SYSTEM_REMOVE;
            this->payload[0] = 0;
        }

        void setPath(const Path& path)
        {
            etl::string_ext(path, (char*)this->payload, PAYLOAD_SIZE);
        }

        inline etl::string_view getPath() const
        {
            return etl::string_view((char*)this->payload);
        }

        void setStatus(filesystem::Status status)
        {
            this->status = status;
        }

        inline filesystem::Status getStatus() const
        {
            return static_cast<filesystem::Status>(this->status);
        }
    };

    struct SystemRename : public os::Message
    {
        static constexpr size_t OLD_PATH_INDEX = 0;
        static constexpr size_t NEW_PATH_INDEX = PAYLOAD_SIZE / 2;

        SystemRename() 
        {
            this->type = filesystem::message::Id::SYSTEM_RENAME;
            this->payload[0] = 0;
        }

        void setOldPath(const Path& path)
        {
            etl::string_ext(path, (char*)this->payload + OLD_PATH_INDEX, PAYLOAD_SIZE / 2);
        }

        inline etl::string_view getOldPath() const
        {
            return etl::string_view((char*)this->payload + OLD_PATH_INDEX, PAYLOAD_SIZE / 2);
        }

        void setNewPath(const Path& path)
        {
            etl::string_ext(path, (char*)this->payload + NEW_PATH_INDEX, PAYLOAD_SIZE / 2);
        }

        inline etl::string_view getNewPath() const
        {
            return etl::string_view((char*)this->payload + NEW_PATH_INDEX, PAYLOAD_SIZE / 2);
        }

        void setStatus(filesystem::Status status)
        {
            this->status = status;
        }

        inline filesystem::Status getStatus() const
        {
            return static_cast<filesystem::Status>(this->status);
        }
    };

    struct SystemStatfsRequest : public os::Message
    {
        SystemStatfsRequest()
        {
            this->type = filesystem::message::Id::SYSTEM_STATFS;
            this->payload[0] = 0;
        }

        void setPath(const Path& path)
        {
            etl::string_ext(path, (char*)this->payload, PAYLOAD_SIZE);
        }

        inline etl::string_view getPath() const
        {
            return etl::string_view((char*)this->payload);
        }
    };
}