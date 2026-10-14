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
    struct Read : public ipc::Message
    {
        using Length = ipc::Message::Input::ReadWrite::Length;

        Read(ipc::Oid file, Length maxLength, 
            etl::span<uint8_t> buffer)
        {
            this->recipient = file;
            this->type = msg::Type::FILE_READ;
            this->input.readWrite.length = maxLength;
            this->output.data = buffer.data();
            this->output.size = buffer.size();
        }

        inline Length getInputLength() const
        {
            return this->input.readWrite.length;
        }

        inline Length getOutputLength() const
        {
            return this->output.readWrite.length;
        }

        inline Read& setOutputLength(Length length)
        {
            this->output.readWrite.length = length;
            return *this;
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline ipc::Oid getFile() const
        {
            return this->recipient;
        }

        inline Read& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }

        inline etl::span<uint8_t> getBuffer() const
        {
            return etl::span<uint8_t>(this->output.data, this->output.size);
        }
    };
}
