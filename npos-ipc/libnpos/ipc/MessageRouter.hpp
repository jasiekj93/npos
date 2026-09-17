#pragma once

/**
 * @file MessageRouter.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <libnpos/ipc/Message.hpp>

namespace npos::ipc
{
    class MessageRouter
    {
    public:
        using Id = uint8_t;

        static constexpr Id BROADCAST = 255;

        explicit MessageRouter(Id id)
            : id(id)
        {
        }

        virtual void receive(const Message& message)
        {
            if(accepts(message.getId()))
                onReceive(message);
        }

        virtual void receive(MessageRouter::Id destinationId, const Message& message)
        {
            if(destinationId == id)
                receive(message);
        }

        inline auto getId() const { return id; }
        inline void setId(Id newId) { id = newId; }

        virtual void onReceive(const Message&) = 0;
        virtual bool accepts(Message::Id) const = 0;
    
    private:
        Id id;
    };
}