#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-10-2026
 */

#include <etl/span.h>

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct Hash : public ipc::Message
    {
        enum Operation : ipc::Message::Input::Attributes::Mode
        {
            ACCUMULATE,
            ACCUMULATE_END
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS,
            DEVICE_FAILURE,
            BUSY,
            NOT_INITIALIZED,
            INVALID_OUTPUT_SIZE
        };

        Hash(ipc::Oid oid, Operation operation, etl::span<const uint8_t> input, etl::span<uint8_t> output = {})
        {
            this->recipient = oid;
            this->type = msg::Type::HASH;
            this->input.readWrite.mode = operation;
            this->input.readWrite.offset = 0;
            this->input.readWrite.length = input.size();
            this->input.data = input.data();
            this->input.size = input.size();

            if(operation == ACCUMULATE_END)
            {
                this->output.data = output.data();
                this->output.size = output.size();
            }
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.readWrite.mode);
        }

        inline etl::span<const uint8_t> getInput() const
        {
            return etl::span<const uint8_t>(this->input.data, this->input.size);
        }

        inline etl::span<uint8_t> getOutput() const
        {
            return etl::span<uint8_t>(this->output.data, this->output.size);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline Hash& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}