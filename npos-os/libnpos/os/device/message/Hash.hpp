#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-09-2026
 */

#include <etl/span.h>
#include <etl/array.h>

#include <libnpos/os/Message.hpp>
#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct Hash
    {
        struct ComputeRequest : public os::Message
        {
            static constexpr auto ID = device::message::Id::HASH_COMPUTE_REQUEST;

            etl::span<const uint8_t> buffer;

            ComputeRequest() : os::Message(ID) {}
        };

        struct ComputeResponse : public os::Message
        {
            static constexpr auto ID = device::message::Id::HASH_COMPUTE_RESPONSE;
            static constexpr size_t HASH_SIZE = 32;

            using Hash = etl::array<uint8_t, HASH_SIZE>;

            enum class Status : uint8_t
            {
                SUCCESS = 0x00,
                COMPUTE_ERROR = 0x01,
                BUSY = 0x02,
            };

            Hash hash;
            Status status;

            ComputeResponse() : os::Message(ID) {}
        };
    };
}