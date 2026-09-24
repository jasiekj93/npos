#pragma once

/**
 * @file System.hpp
 * @author Adrian Szczepanski
 * @date 03-09-2026
 */

#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct System
    {
        struct ResetRequest : public os::Message
        {
            static constexpr auto ID = device::message::Id::SYSTEM_RESET_REQUEST;

            ResetRequest() : os::Message(ID) {}
        };
    };
}