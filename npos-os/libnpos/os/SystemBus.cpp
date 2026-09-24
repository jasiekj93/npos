#include "SystemBus.hpp"

using namespace npos;
using namespace npos::os;

SystemBus::SystemBus(Service::Id startIndex, ServiceList& services)
    : ipc::MessageRouter(BROADCAST)
    , startId(startIndex)
    , services(services)
    , successor(nullptr)
{
}

bool SystemBus::subscribe(Service& service)
{
    if (services.full())
        return false;

    service.setId(getServiceId(services.size()));
    services.push_back(&service);
    return true;
}

void SystemBus::receive(ipc::MessageRouter::Id recipient, const ipc::Message& message)
{
    if(recipient == BROADCAST)
        return receive(message);

    if(recipient < startId)
        return;

    auto index = getIndex(static_cast<Service::Id>(recipient));

    if(index < services.size())
        services[index]->receive(message);
    else if(successor)
        successor->receive(recipient, message);
}

void SystemBus::receive(const ipc::Message& message)
{
    for(auto& service : services)
        service->receive(message);

    if(successor)
        successor->receive(message);
}

size_t SystemBus::getIndex(Service::Id id) const
{
    auto index = static_cast<size_t>(id) - static_cast<size_t>(startId);
    return index;
}

Service::Id SystemBus::getServiceId(size_t index) const
{
    return static_cast<Service::Id>(static_cast<size_t>(startId) + index);
}