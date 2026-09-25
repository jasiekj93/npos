#include "SystemBus.hpp"

using namespace npos;
using namespace npos::os;

SystemBus::SystemBus(Pid startIndex, ServiceList& services, MessagePool& messagePool)
    : Bus()
    , startId(startIndex)
    , services(services)
    , successor(nullptr)
    , messagePool(messagePool)
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

void SystemBus::sendTo(Pid recipient, const Message& message)
{
    if(recipient < startId)
        return;

    auto index = getIndex(recipient);

    if(index < services.size())
    {
        if(services[index]->accepts(message.type))
        {
            auto* messagePtr = const_cast<Message*>(&message);

            if(not messagePool.is_in_pool(messagePtr))
            {
                if(messagePool.full())
                {
                    //TODO handle full message pool scenario
                    return;
                }

                messagePtr = new (messagePool.allocate<Message>()) Message(message);
            }

            messagePtr->referenceCount++;
            services[index]->onReceive(*messagePtr);
        }
    }
    else if(successor)
        successor->sendTo(recipient, message);
}

void SystemBus::broadcast(const Message& message)
{
    if(messagePool.full())
    {
        //TODO handle full message pool scenario
        return;
    }

    auto* allocatedMessage = new (messagePool.allocate<Message>()) Message(message);

    for(auto& service : services)
    {
        if(service->accepts(message.type))
        {
            allocatedMessage->referenceCount++;
            service->onReceive(*allocatedMessage);
        }
    }

    if(allocatedMessage->referenceCount == 0)
        messagePool.release(allocatedMessage);

    if(successor)
        successor->broadcast(message);
}

void SystemBus::respond(const Message& message)
{
    if(message.sender < startId)
        return;

    auto index = getIndex(message.sender);

    if(index < services.size())
    {
        if(services[index]->accepts(message.type))
        {
            auto* response = const_cast<Message*>(&message);

            if(not messagePool.is_in_pool(response))
            {
                if(messagePool.full())
                {
                    //TODO handle full message pool scenario
                    return;
                }

                response = new (messagePool.allocate<Message>()) Message(message);
            }

            response->referenceCount++;
            services[index]->onReceive(*response);
        }
    }
    else if(successor)
        successor->respond(message);
}

void SystemBus::release(const Message const* message)
{
    if(not messagePool.is_in_pool(message))
        return;

    if(message->referenceCount == 0)
        return messagePool.release(message);

    auto* ptr = const_cast<Message*>(message);
    ptr->referenceCount--;

    if(ptr->referenceCount == 0)
        messagePool.release(ptr);
}

size_t SystemBus::getIndex(Pid id) const
{
    auto index = static_cast<size_t>(id) - static_cast<size_t>(startId);
    return index;
}

Pid SystemBus::getServiceId(size_t index) const
{
    return static_cast<Pid>(static_cast<size_t>(startId) + index);
}