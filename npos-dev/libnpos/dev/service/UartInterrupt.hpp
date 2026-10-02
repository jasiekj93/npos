#pragma once

/**
 * @file UartInterrupt.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/dev/InterruptService.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/hal/Uart.hpp>

namespace npos::dev::service
{
    class UartInterrupt : public InterruptService
    {
    public:
        UartInterrupt(Api& api, InterruptQueueInt& interruptQueue, hal::UartInterrupt& uart, etl::string_view name)
            : InterruptService(api, interruptQueue, uart)
            , uart(uart)
            , name(name)
        {
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == msg::Type::TRANSCIEVER)
            {
                auto& request = static_cast<msg::Transciever&>(message);

                if(not isInitalized)
                    return getApi().respond(request.setStatus(msg::Transciever::Status::NOT_INITIALIZED));

                if(request.getOperation() == msg::Transciever::Operation::READ)
                {
                    if(readRequest != nullptr)
                        return getApi().respond(request.setStatus(msg::Transciever::Status::BUSY));

                    request.referenceCount++;
                    readRequest = &request;

                    auto buffer = request.getOutputBuffer();
                    auto result = uart.receiveIt(buffer.data(), request.getLength());

                    if(not result)
                    {
                        request.referenceCount--;
                        readRequest = nullptr;
                        return getApi().respond(request.setStatus(msg::Transciever::Status::DEVICE_ERROR));
                    }
                }
                else if(request.getOperation() == msg::Transciever::Operation::WRITE)
                {
                    if(writeRequest != nullptr)
                        return getApi().respond(request.setStatus(msg::Transciever::Status::BUSY));

                    request.referenceCount++;
                    writeRequest = &request;
                    
                    auto buffer = request.getInputBuffer();
                    auto result = uart.transmitIt(buffer.data(), buffer.size());

                    if(not result)
                    {
                        request.referenceCount--;
                        writeRequest = nullptr;
                        return getApi().respond(request.setStatus(msg::Transciever::Status::DEVICE_ERROR));
                    }
                }

            }
            else if (message.type == msg::Type::OPEN)
                open(message);
        }

        bool accepts(ipc::Message::Type type) const override
        {
            return (type == msg::Type::TRANSCIEVER or 
                    type == msg::Type::OPEN);
        }

        void onInterrupt(const hal::Interrupt& interrupt) override
        {
            if(interrupt.type != hal::Interrupt::Type::UART)
                return;

            switch (interrupt.code)
            {
            case hal::UartInterrupt::Event::TRANSMIT_COMPLETE:
            {
                if(writeRequest == nullptr)
                    return;

                auto* request = writeRequest;
                request->referenceCount--;
                writeRequest = nullptr;
                return getApi().respond(request->setStatus(msg::Transciever::Status::SUCCESS));
            }

            case hal::UartInterrupt::Event::RECEIVE_COMPLETE:
            {
                if(readRequest == nullptr)
                    return;

                auto* request = readRequest;
                request->referenceCount--;
                readRequest = nullptr;
                return getApi().respond(request->setStatus(msg::Transciever::Status::SUCCESS));
            }
            
            case hal::UartInterrupt::Event::ERROR:
            {
                if(readRequest != nullptr)
                {
                    auto* request = readRequest;
                    request->referenceCount--;
                    readRequest = nullptr;
                    return getApi().respond(request->setStatus(msg::Transciever::Status::DEVICE_ERROR));
                }

                if(writeRequest != nullptr)
                {
                    auto* request = writeRequest;
                    request->referenceCount--;
                    writeRequest = nullptr;
                    return getApi().respond(request->setStatus(msg::Transciever::Status::DEVICE_ERROR));
                }
            }
            default:
                break;
            }
        }

    protected:
        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            auto status = msg::Open::Status::SUCCESS;

            if(not isInitalized)
            {
                if(not uart.init())
                    status = msg::Open::Status::INIT_FAILURE;
                else
                    isInitalized = true;
            }

            api.respond(request.setStatus(status).setOpened(getOid()));
        }

    private:
        hal::UartInterrupt& uart;
        etl::string_view name;
        bool isInitalized = false;
        dev::msg::Transciever* readRequest = nullptr;
        dev::msg::Transciever* writeRequest = nullptr;
    };
}