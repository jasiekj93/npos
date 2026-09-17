#pragma once

#include <libnpos/os/message/RequestResponse.hpp>
#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct LedRequest : public ipc::Message
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

        LedRequest(LedId ledId, Operation operation) 
            : ipc::Message(ID)
            , ledId(ledId)
            , operation(operation) 
        {}
    };
}