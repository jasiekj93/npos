#pragma once

/**
 * @file Transciever.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <etl/string.h>

#include <libnpos/device/Type.hpp>
#include <libnpos/device/Status.hpp>

namespace npos::device
{
    struct TranscieverOpen : public kernel::Message
    {
        TranscieverOpen(etl::string_view name)
        {
            this->type = device::Type::TRANSCEIVER_OPEN;
            this->data = reinterpret_cast<uint8_t*>(const_cast<char*>(name.data()));
            this->size = name.size();
        }

        inline etl::string_view getName() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->data), this->size);
        }

        inline TranscieverOpen& setStatus(device::Status status)
        {
            this->status = status;
            return *this;
        }

        inline device::Status getStatus() const
        {
            return static_cast<device::Status>(this->status);
        }

        
    };

    struct ReadRequest : public kernel::Message
    {

    };

    struct WriteRequest : public kernel::Message
    {

    };
}