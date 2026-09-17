#pragma once

/**
 * @file Hash.hpp
 * @author Adrian Szczepanski
 * @date 01-09-2026
 */

#include <etl/span.h>

#include <libnpos/os/driver/message/Id.hpp>
#include <libnpos/os/driver/hal/Interruptable.hpp>

namespace npos::os::driver::hal
{
    class Hash : public Interruptable
    {
    public:
        struct Interrupt : public ::npos::ipc::Message
        {
            static constexpr auto ID = ::npos::os::driver::message::Id::HASH_INTERRUPT;
            enum Type : uint8_t
            {
                INPUT_COMPLETE = 0x00,
                DIGEST_COMPLETE = 0x01,
                ERROR = 0x02
            };

            Type type;
        };

        virtual ~Hash() = default;

        virtual bool init() = 0;
        virtual void deinit() = 0;

        virtual bool accumulate(etl::span<const uint8_t> input) = 0;
        virtual bool accumulateEnd(etl::span<const uint8_t> input, etl::span<uint8_t> output) = 0;
    };
}