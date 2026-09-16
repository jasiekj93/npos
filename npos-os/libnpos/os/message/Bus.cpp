#include "Bus.hpp"

using namespace npos;
using namespace npos::os;
using namespace npos::os::message;

Bus::Bus() 
    : ipc::MessageRouter(ipc::MessageRouter::BROADCAST) 
{
}

bool Bus::subscribe(ipc::MessageRouter& router)
{
    if(routerList.full())
        return false;

    auto newId = routerList.size();
    router.setId(newId);
    routerList.push_back(&router);

    return true;
}

void Bus::receive(const ipc::Message& message)
{
    for(auto& router : routerList)
            router->receive(message);
}

void Bus::receive(ipc::MessageRouter::Id destinationId, const ipc::Message& message)
{
    if(destinationId == BROADCAST)
        return receive(message);

    if(destinationId < routerList.size())
    {
        if(routerList[destinationId]->getId() == destinationId)
            routerList[destinationId]->receive(message);
    }
}
