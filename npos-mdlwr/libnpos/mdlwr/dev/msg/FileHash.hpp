#pragma once

/**
 * @file FileHash.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <etl/string.h>
#include <etl/span.h>

#include <libnpos/mdlwr/dev/msg/Type.hpp>

namespace npos::mdlwr::dev::msg
{
    struct FileHash : public ipc::Message
    {
        enum Status : ipc::Message::Status
        {
            SUCCESS = 0,
            DEVICE_ERROR,
            BUFFER_TOO_SMALL,
            HASH_NOT_FOUND,
            HASH_BUSY,
            FILE_NOT_FOUND,
            IS_A_DIRECTORY,
            INVALID_PATH,
            BUSY
        };

        FileHash(etl::string_view path, etl::span<uint8_t> outputBuffer)
        {
            this->type = msg::Type::FILE_HASH;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
            this->output.data = outputBuffer.data();
            this->output.size = outputBuffer.size();
        }

        etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        etl::span<uint8_t> getOutputBuffer() const
        {
            return etl::span<uint8_t>(this->output.data, this->output.size);
        }

        Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        FileHash& setStatus(Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}