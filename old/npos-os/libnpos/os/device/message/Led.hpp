#pragma once

/**
 * @file Led.hpp
 * @author Adrian Szczepanski
 * @date 24-09-2026
 */

#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct LedRequest : public os::Message
    {
        static constexpr auto ID = device::message::Id::LED_SET_STATE_REQUEST;

        using LedId = uint8_t;

        enum Operation
        {
            OFF,
            ON,
            TOGGLE
        };

        LedId ledId;
        Operation operation;

        LedRequest() : os::Message(ID) {}
    };
}