#pragma once

/**
 * @file Transciever.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <etl/span.h>

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct Transciever : public ipc::Message
    {
        enum Operation : ipc::Message::Input::ReadWrite::Mode
        {
            READ,
            WRITE
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS,
            BUSY,
            NOT_INITIALIZED,
            DEVICE_ERROR
        };

        using Length = ipc::Message::Input::ReadWrite::Length;

        Transciever(ipc::Oid oid, Operation operation, Length length, etl::span<uint8_t> buffer)
        {
            this->recipient = oid;
            this->type = msg::Type::TRANSCIEVER;
            this->input.readWrite.mode = operation;
            this->input.readWrite.length = length;

            if(operation == READ)
            {
                this->output.data = buffer.data();
                this->output.size = length;
            }
            else
            {
                this->input.data = buffer.data();
                this->input.size = length;
            }
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.readWrite.mode);
        }

        inline Length getLength() const
        {
            return this->input.readWrite.length;
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Transciever& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }

        inline etl::span<const uint8_t> getInputBuffer() const
        {
            return etl::span<const uint8_t>(this->input.data, this->input.size);
        }

        inline etl::span<uint8_t> getOutputBuffer() const
        {
            return etl::span<uint8_t>(this->output.data, this->output.size);
        }
    };
}