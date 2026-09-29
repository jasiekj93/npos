#include "UartInterrupt.hpp"

#include <libnpos/device/Error.hpp>

using namespace npos;
using namespace npos::driver;
using namespace npos::driver::service;

UartInterrupt::UartInterrupt(hal::UartInterrupt& uart, kernel::Bus& bus, device::TranscieverRequest::Id id)
    : kernel::Service(bus)
    , uart(uart)
    , id(id)
    , receiveRequest(nullptr)
    , transmitRequest(nullptr)
{
}

void UartInterrupt::initalize()
{
    if(not uart.init())
        broadcast(device::Error(device::Error::Code::UART_INITIALIZATION, id));
}

bool UartInterrupt::accepts(kernel::Message::Type type) const
{
    return type == device::Type::TRANSCEIVER;
}

void UartInterrupt::handleInterrupt(const hal::Interrupt& interrupt)
{
    if(interrupt.type != hal::Interrupt::Type::UART)
        return;

    if(interrupt.code == hal::UartInterrupt::Event::RECEIVE_COMPLETE)
    {
        if(receiveRequest == nullptr)
            return;

        receiveRequest->setStatus(device::Status::OK).setOutputData(etl::span<uint8_t>(receiveRequest->getInputData()));
        receiveRequest->referenceCount--;
        respond(*receiveRequest);
        receiveRequest = nullptr;
    }
    else if(interrupt.code == hal::UartInterrupt::Event::TRANSMIT_COMPLETE)
    {
        if(transmitRequest == nullptr)
            return;

        transmitRequest->setStatus(device::Status::OK).setOutputData(etl::span<uint8_t>(transmitRequest->getInputData()));
        transmitRequest->referenceCount--;
        respond(*transmitRequest);
        transmitRequest = nullptr;
    }
    else if(interrupt.code == hal::UartInterrupt::Event::ERROR)
    {
        if(receiveRequest != nullptr)
        {
            receiveRequest->setStatus(device::Status::DEVICE_ERROR);
            receiveRequest->referenceCount--;
            respond(*receiveRequest);
            receiveRequest = nullptr;
        }

        if(transmitRequest != nullptr)
        {
            transmitRequest->setStatus(device::Status::DEVICE_ERROR);
            transmitRequest->referenceCount--;
            respond(*transmitRequest);
            transmitRequest = nullptr;
        }
    }
}

void UartInterrupt::onReceive(kernel::Message& message)
{
    auto& request = static_cast<device::TranscieverRequest&>(message);

    if(request.getId() != id)
        return;

    if(request.input.data == nullptr or request.input.size == 0)
        return respond(request.setStatus(device::Status::INVALID_PARAMETER));

    if(request.getMode() == device::TranscieverRequest::Mode::READ)
    {
        if(receiveRequest != nullptr)
            return respond(request.setStatus(device::Status::DEVICE_BUSY));

        request.referenceCount++;
        receiveRequest = &request;

        if(not uart.receiveIt(const_cast<uint8_t*>(request.input.data), request.input.io.length))
        {
            receiveRequest = nullptr;
            request.referenceCount--;
            return respond(request.setStatus(device::Status::DEVICE_ERROR));
        }
    }
    else
    {
        if(transmitRequest != nullptr)
            return respond(request.setStatus(device::Status::DEVICE_BUSY));

        request.referenceCount++;
        transmitRequest = &request;

        if(not uart.transmitIt(request.input.data, request.input.io.length))
        {
            transmitRequest = nullptr;
            request.referenceCount--;
            return respond(request.setStatus(device::Status::DEVICE_ERROR));
        }
    }
}