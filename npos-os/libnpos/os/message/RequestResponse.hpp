#pragma once

/**
 * @file RequestResponse.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::os::message
{
    struct Request : public ipc::Message
    {
        ipc::MessageRouter::Id senderId;

        Request(ipc::Message::Id id) : ipc::Message(id) {}
    };
    
    using Response = ipc::Message;
}