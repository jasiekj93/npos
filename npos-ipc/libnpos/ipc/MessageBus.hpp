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

        explicit MessageBus(RouterList& rl)
            : MessageRouter(BROADCAST)
            , routerList(rl)
        {
        }

        bool subscribe(MessageRouter& router)
        {
            if(routerList.full())
                return false;

            routerList.push_back(&router);
            return true;
        }

        void receive(const Message& message)
        {
            for(auto& router : routerList)
                    router->receive(message);
        }

        void receive(MessageRouter::Id destinationId, const Message& message)
        {
            if(destinationId == BROADCAST)
                return receive(message);

            for(auto& router : routerList)
            {
                if(destinationId == router->getId())
                    router->receive(message);
            }
        }

        inline void onReceive(const Message&) override {}
        inline bool accepts(Message::Id) const override { return true; }

    private:
        RouterList& routerList;
    };
}