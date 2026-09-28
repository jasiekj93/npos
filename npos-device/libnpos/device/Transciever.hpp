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
    struct TranscieverRequest : public kernel::Message
    {
        enum Mode : kernel::Message::InputOutput::Mode
        {
            READ = 0,
            WRITE,
        };

        using Id = kernel::Message::ObjectId;

        TranscieverRequest(Id id, Mode mode, etl::span<uint8_t> buffer)
        {
            this->type = device::Type::TRANSCEIVER;
            this->object = id;
            this->inputOutput.mode = mode;
            this->data = buffer.data();
            this->size = buffer.size();
        }

        inline Id getId() const
        {
            return this->object;
        }

        inline Mode getMode() const
        {
            return static_cast<Mode>(this->inputOutput.mode);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->status);
        }

        inline TranscieverRequest& setStatus(Status status)
        {
            this->status = static_cast<kernel::Message::Status>(status);
            return *this;
        }

        inline TranscieverRequest& setLength(size_t length)
        {
            this->inputOutput.length = length;
            return *this;
        }   
    };
}