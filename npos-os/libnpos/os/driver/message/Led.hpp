#pragma once

#include <libnpos/os/message/RequestResponse.hpp>
#include <libnpos/os/driver/message/Id.hpp>

namespace npos::os::driver::message
{
    struct LedRequest : public os::message::Request
    {
        static constexpr auto ID = Id::LED_SET_STATE_REQUEST;

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
            : os::message::Request(ID)
            , ledId(ledId)
            , operation(operation) 
        {}
    };
}