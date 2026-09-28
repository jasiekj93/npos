#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/device/Type.hpp>
#include <libnpos/device/Status.hpp>

namespace npos::device
{
    struct LedRequest : public kernel::Message
    {
        using LedId = uint8_t;

        enum Operation
        {
            OFF,
            ON,
            TOGGLE
        };

        LedRequest(LedId ledId, Operation operation)
        {
            this->type = device::Type::LED; 
            this->attributes.type = static_cast<uint8_t>(operation);
            this->attributes.value = static_cast<uint8_t>(ledId);
        }

        inline LedId getLedId() const
        {
            return static_cast<LedId>(this->attributes.value);
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->attributes.type);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->status);
        }

        inline LedRequest& setStatus(Status status)
        {
            this->status = static_cast<kernel::Message::Status>(status);
            return *this;
        }
    };
}