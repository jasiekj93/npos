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

        explicit MessageRouter(Id);

        virtual void receive(const Message& message);
        virtual void receive(MessageRouter::Id destinationId, const Message& message);

        inline auto getId() const { return id; }
    
    protected:
        virtual void onReceive(const Message&) = 0;
        virtual bool accepts(Message::Id) = 0;

    private:
        Id id;
    };
}