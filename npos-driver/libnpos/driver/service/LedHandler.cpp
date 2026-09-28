#include "LedHandler.hpp"

using namespace npos;
using namespace npos::driver;
using namespace npos::driver::service;

LedHandler::LedHandler(hal::Led& led, kernel::Bus& bus)
    : kernel::Service(bus)
    , led(led)
{
}

void LedHandler::onReceive(kernel::Message& message)
{
    auto& request = static_cast<device::LedRequest&>(message);

    if(not led.available(request.getLedId()))
        return respond(request.setStatus(device::Status::INVALID_PARAMETER));

    switch(request.getOperation())
    {
        case device::LedRequest::OFF:
            led.off(request.getLedId());
            break;
        case device::LedRequest::ON:
            led.on(request.getLedId());
            break;
        case device::LedRequest::TOGGLE:
            led.toggle(request.getLedId());
            break;
        default:
            return respond(request.setStatus(device::Status::INVALID_OPERATION));
            break;
    }
}

bool LedHandler::accepts(kernel::Message::Type type) const
{
    return (type == device::Type::LED);
}
