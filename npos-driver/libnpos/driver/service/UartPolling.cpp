#include "UartPolling.hpp"

#include <libnpos/device/Error.hpp>

using namespace npos;
using namespace npos::driver;
using namespace npos::driver::service;

UartPolling::UartPolling(hal::UartPolling& uart, kernel::Bus& bus, device::TranscieverRequest::Id id)
    : kernel::Service(bus)
    , uart(uart)
    , id(id)
{
}

void UartPolling::initalize()
{
    if(not uart.init())
        broadcast(device::Error(device::Error::Code::UART_INITIALIZATION, id));
}

bool UartPolling::accepts(kernel::Message::Type type) const
{
    return type == device::Type::TRANSCEIVER;
}

void UartPolling::onReceive(kernel::Message& message)
{
    if(request.getId() != id)
        return;

    auto& request = static_cast<device::TranscieverRequest&>(message);
    bool result = false;

    if(request.getMode() == device::TranscieverRequest::Mode::READ)
        result = uart.receive(request.data, request.size);
    else
        result = uart.transmit(request.data, request.size);

    if(not result)
        respond(request.setStatus(device::Status::DEVICE_ERROR));
    else
        respond(request.setStatus(device::Status::OK).setLength(request.size));
}