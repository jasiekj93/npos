#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-09-2026
 */

#include <etl/span.h>
#include <etl/array.h>

#include <libnpos/os/message/RequestResponse.hpp>
#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct Hash
    {
        struct ComputeRequest : public os::message::Request
        {
            static constexpr auto ID = device::message::Id::HASH_COMPUTE_REQUEST;

            etl::span<const uint8_t> buffer;

            ComputeRequest(Request::SenderId senderId, etl::span<const uint8_t> buffer)
                : os::message::Request(ID, senderId)
                , buffer(buffer) {}
        };

        struct ComputeResponse : public os::message::Response
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

            ComputeResponse(Request::SenderId senderId, const Hash& hash, Status status)
                : os::message::Response(ID)
                , hash(hash)
                , status(status) {}
        };
    };
}