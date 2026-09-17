#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 03-09-2026
 */

#include <libnpos/os/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::device::message
{
    struct System
    {
        struct ResetRequest : public os::message::Request
        {
            static constexpr auto ID = device::message::Id::SYSTEM_RESET_REQUEST;

            ResetRequest(Request::SenderId senderId)
                : os::message::Request(ID, senderId) {}
        };
    };
}