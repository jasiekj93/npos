#include "MessageBus.hpp"

using namespace npos::ipc;

MessageBus::MessageBus(RouterList& rl)
    : MessageRouter(BROADCAST)
    , routerList(rl)
{
}

bool MessageBus::subscribe(MessageRouter& router)
{
    if(routerList.full())
        return false;

    routerList.push_back(&router);
    return true;
}

void MessageBus::receive(const Message& message)
{
    for(auto& router : routerList)
            router->receive(message);
}

void MessageBus::receive(MessageRouter::Id destinationId, const Message& message)
{
    if(destinationId == BROADCAST)
        return receive(message);

    for(auto& router : routerList)
    {
        if(destinationId == router->getId())
            router->receive(message);
    }
}
