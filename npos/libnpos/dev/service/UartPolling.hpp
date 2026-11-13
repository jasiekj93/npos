#pragma once

/**
 * @file UartPolling.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Close.hpp>
#include <libnpos/dev/hal/Uart.hpp>

namespace npos::dev::service
{
    class UartPolling : public ipc::Service
    {
    public:
        UartPolling(Api& api, hal::UartPolling& uart, etl::string_view name)
            : ipc::Service(api)
            , uart(uart)
            , name(name)
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

                if(owner != request.sender)
                    return getApi().respond(request.setStatus(msg::Transciever::Status::LOCKED));

                if(request.getOperation() == msg::Transciever::Operation::READ)
                    return read(request);    
                else if(request.getOperation() == msg::Transciever::Operation::WRITE)
                    return write(request);
                else
                    return handleUnknownRequest(message);
            }
            else if(message.type == msg::Type::OPEN)
                open(message);
            else if(message.type == msg::Type::CLOSE)
                close(message);
        }
        
        void handleResponse(ipc::Message& message) override {}

        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::TRANSCIEVER or 
                    type == msg::Type::OPEN or
                    type == msg::Type::CLOSE);
        }

    protected:
        void read(msg::Transciever& request)
        {
            auto buffer = request.getOutputBuffer();
            auto result = uart.receive(buffer.data(), request.getInputLength());
            auto status = (result == true ? msg::Transciever::Status::SUCCESS : msg::Transciever::Status::DEVICE_ERROR);

            getApi().respond(request.setStatus(status).setOutputLength(result ? request.getInputLength() : 0));
        }

        void write(msg::Transciever& request)
        {
            auto buffer = request.getInputBuffer();
            auto result = uart.transmit(buffer.data(), buffer.size());
            auto status = (result == true ? msg::Transciever::Status::SUCCESS : msg::Transciever::Status::DEVICE_ERROR);

            getApi().respond(request.setStatus(status).setOutputLength(result ? request.getInputLength() : 0));
        }

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
        hal::UartPolling& uart;
        etl::string_view name;
        ipc::Oid owner = ipc::NULL_OID;
    };
}
