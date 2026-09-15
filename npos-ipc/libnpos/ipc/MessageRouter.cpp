#include "MessageRouter.hpp"

using namespace npos::ipc;

MessageRouter::MessageRouter(Id id)
    : id(id)
{
}

void MessageRouter::receive(const Message &message)
{
    if(accepts(message.getId()))
        onReceive(message);
}

void MessageRouter::receive(MessageRouter::Id destinationId, const Message& message)
{
    if(destinationId == id)
        receive(message);
}
