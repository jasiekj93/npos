#pragma once

/**
 * @file Read.hpp
 * @author Adrian Szczepanski
 * @date 12-10-2026
 */

#include <etl/span.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg::file
{
    struct Write : public ipc::Message
    {
        using Length = ipc::Message::Input::ReadWrite::Length;

        Write(ipc::Oid file, etl::span<const uint8_t> buffer)
        {
            this->recipient = file;
            this->type = msg::Type::FILE_WRITE;
            this->input.readWrite.length = buffer.size();
            this->input.data = buffer.data();
            this->input.size = buffer.size();
        }

        inline Length getInputLength() const
        {
            return this->input.readWrite.length;
        }

        inline Length getOutputLength() const
        {
            return this->output.readWrite.length;
        }

        inline Write& setOutputLength(Length length)
        {
            this->output.readWrite.length = length;
            return *this;
        }

        inline ipc::Oid getFile() const
        {
            return this->recipient;
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Write& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }

        inline etl::span<const uint8_t> getBuffer() const
        {
            return etl::span<const uint8_t>(this->input.data, this->input.size);
        }
    };
}
