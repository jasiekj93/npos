#include "LedHandler.hpp"

using namespace npos;
using namespace npos::os;
using namespace npos::os::driver;
using namespace npos::os::driver::service;

LedHandler::LedHandler(hal::Led& led, os::message::Bus& bus)
    : os::Service(bus)
    , led(led)
{
}

void LedHandler::initalize()
{
}

void LedHandler::onReceive(const ipc::Message& message)
{
    auto& request = static_cast<const message::LedRequest&>(message);

    if(not led.available(request.ledId))
    {
        //todo syslog
        return;
    }

    switch(request.operation)
    {
        case message::LedRequest::OFF:
            led.off(request.ledId);
            break;
        case message::LedRequest::ON:
            led.on(request.ledId);
            break;
        case message::LedRequest::TOGGLE:
            led.toggle(request.ledId);
            break;
    }
}

bool LedHandler::accepts(ipc::Message::Id id) const
{
    return (id == message::LedRequest::ID);
}
