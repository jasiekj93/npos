#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 28-09-2026
 */

#include <etl/span.h>

#include <libnpos/device/Type.hpp>

namespace npos::device
{
    struct HashRequest : public kernel::Message
    {
        HashRequest(etl::span<const uint8_t> input, etl::span<uint8_t> output)
        {
            this->data = data;
        }
    }
}