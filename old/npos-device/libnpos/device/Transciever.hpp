#pragma once

/**
 * @file Transciever.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <etl/span.h>

#include <libnpos/device/Type.hpp>
#include <libnpos/device/Status.hpp>

namespace npos::device
{
    struct TranscieverReadRequest : public kernel::Message
    {
        enum Mode : kernel::Message::Input::IO::Mode
        {
            READ = 0,
            WRITE,
        };

        using Id = kernel::Message::ObjectId;

        TranscieverRequest(Id id, Mode mode, etl::span<uint8_t> inputBuffer, size_t length)
        {
            this->type = device::Type::TRANSCEIVER;
            this->object = id;
            this->input.io.mode = mode;
            this->input.io.length = length;
            this->input.data = inputBuffer.data();
            this->input.size = inputBuffer.size();
        }

        inline Id getId() const
        {
            return this->object;
        }

        inline etl::span<const uint8_t> getInputData() const
        {
            return etl::span<const uint8_t>(this->input.data, this->input.size);
        }

        inline Mode getMode() const
        {
            return static_cast<Mode>(this->input.io.mode);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline TranscieverRequest& setStatus(Status status)
        {
            this->output.status = static_cast<kernel::Message::Status>(status);
            return *this;
        }

        inline TranscieverRequest& setOutputData(etl::span<uint8_t> buffer)
        {
            this->output.data = buffer.data();
            this->output.size = buffer.size();
            return *this;
        }
    };
}