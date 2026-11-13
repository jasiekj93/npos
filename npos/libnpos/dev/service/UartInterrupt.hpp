#pragma once

/**
 * @file UartInterrupt.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/MessageStore.hpp>
#include <libnpos/dev/InterruptService.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Close.hpp>
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
            , readRequest(*this)
            , writeRequest(*this)
            , owner(ipc::NULL_OID)
        {
        }

        void handleRequest(ipc::Message& message) override
        {
            if(message.type == msg::Type::TRANSCIEVER)
            {
                auto& request = static_cast<msg::Transciever&>(message);

                if(owner == ipc::NULL_OID)
                    return getApi().respond(request.setStatus(msg::Transciever::Status::NOT_INITIALIZED));

                if(request.getOperation() == msg::Transciever::Operation::READ)
                {
                    if(owner != request.sender)
                        return getApi().respond(request.setStatus(msg::Transciever::Status::LOCKED));

                    if(readRequest.hasValue())
                        return getApi().respond(request.setStatus(msg::Transciever::Status::BUSY).setOutputLength(0));

                    readRequest.store(request);

                    auto buffer = request.getOutputBuffer();
                    auto result = uart.receiveIt(buffer.data(), request.getInputLength());

                    if(not result)
                    {
                        readRequest.release();
                        return getApi().respond(request.setStatus(msg::Transciever::Status::DEVICE_ERROR).setOutputLength(0));
                    }
                }
                else if(request.getOperation() == msg::Transciever::Operation::WRITE)
                {
                    if(owner != request.sender)
                        return getApi().respond(request.setStatus(msg::Transciever::Status::LOCKED));

                    if(writeRequest.hasValue())
                        return getApi().respond(request.setStatus(msg::Transciever::Status::BUSY).setOutputLength(0));

                    writeRequest.store(request);
                    
                    auto buffer = request.getInputBuffer();
                    auto result = uart.transmitIt(buffer.data(), buffer.size());

                    if(not result)
                    {
                        writeRequest.release();
                        return getApi().respond(request.setStatus(msg::Transciever::Status::DEVICE_ERROR).setOutputLength(0));
                    }
                }
                else
                    return handleUnknownRequest(message);

            }
            else if (message.type == msg::Type::OPEN)
                open(message);
            else if (message.type == msg::Type::CLOSE)
                close(message);
        }

        void handleResponse(ipc::Message& message) override {}

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::TRANSCIEVER or 
                    type == msg::Type::OPEN or
                    type == msg::Type::CLOSE);
        }

        void onInterrupt(const hal::Interrupt& interrupt) override
        {
            if(interrupt.type != hal::Interrupt::Type::UART)
                return;

            switch (interrupt.code)
            {
            case hal::UartInterrupt::Event::TRANSMIT_COMPLETE:
            {
                if(not writeRequest.hasValue())
                    return;

                auto* request = writeRequest.release();
                return getApi().respond(request->setStatus(msg::Transciever::Status::SUCCESS).setOutputLength(request->getInputLength()));
            }

            case hal::UartInterrupt::Event::RECEIVE_COMPLETE:
            {
                if(not readRequest.hasValue())
                    return;

                auto* request = readRequest.release();
                return getApi().respond(request->setStatus(msg::Transciever::Status::SUCCESS).setOutputLength(interrupt.value));
            }
            
            case hal::UartInterrupt::Event::ERROR:
            {
                if(readRequest.hasValue())
                {
                    auto* request = readRequest.release();
                    getApi().respond(request->setStatus(msg::Transciever::Status::DEVICE_ERROR).setOutputLength(0));
                }

                if(writeRequest.hasValue())
                {
                    auto* request = writeRequest.release();
                    getApi().respond(request->setStatus(msg::Transciever::Status::DEVICE_ERROR).setOutputLength(0));
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

            if(owner == request.sender)
                return api.respond(request.setStatus(msg::Open::Status::ALREADY_OPENED));

            if(owner != ipc::NULL_OID)
                return api.respond(request.setStatus(msg::Open::Status::LOCKED));

            auto status = msg::Open::Status::SUCCESS;

            if(not uart.init())
                status = msg::Open::Status::INIT_FAILURE;
            else
                owner = request.sender;

            api.respond(request.setStatus(status).setOpened(getOid()));
        }

        void close(ipc::Message& message)
        {
            auto& request = static_cast<msg::Close&>(message);

            if(owner == ipc::NULL_OID)
                return api.respond(request.setStatus(msg::Close::Status::NOT_INITIALIZED));

            if(owner != message.sender)
                return api.respond(request.setStatus(msg::Close::Status::LOCKED));

            owner = ipc::NULL_OID;
            uart.deinit();
            api.respond(request.setStatus(msg::Close::Status::SUCCESS));
        }

    private:
        hal::UartInterrupt& uart;
        etl::string_view name;
        ipc::Oid owner = ipc::NULL_OID;
        ipc::MessageStore<dev::msg::Transciever> readRequest;
        ipc::MessageStore<dev::msg::Transciever> writeRequest;
    };
}