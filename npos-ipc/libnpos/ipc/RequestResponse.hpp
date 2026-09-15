#pragma once

/**
 * @file RequestResponse.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::ipc
{
    struct Request : public Message
    {
        MessageRouter::Id senderId;

        Request(Message::Id id) : Message(id) {}
    };
    
    using Response = Message;
}