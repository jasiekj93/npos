#pragma once

/**
 * @file MessageBus.hpp
 * @author Adrian Szczepanski
 * @date 15-09-2026
 */

#include <etl/vector.h>

#include <libnpos/ipc/MessageRouter.hpp>

namespace npos::ipc
{
    class MessageBus : public MessageRouter
    {
    public:
        using RouterList = etl::ivector<MessageRouter*>;

        explicit MessageBus(RouterList&);

        bool subscribe(MessageRouter&);

        void receive(const Message& message) override;
        void receive(MessageRouter::Id destinationId, const Message& message) override;

    protected:
        inline void onReceive(const Message&) override {}
        inline bool accepts(Message::Id) override { return true; }

    private:
        RouterList& routerList;
    };
}