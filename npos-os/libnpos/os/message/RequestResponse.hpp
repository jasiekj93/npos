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
        using SenderId = ipc::MessageRouter::Id;

        SenderId senderId;

        Request(ipc::Message::Id id, SenderId senderId) 
            : ipc::Message(id)
            , senderId(senderId) {}
    };
    
    using Response = ipc::Message;
}