#pragma once

/**
 * @file UartPolling.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/Transciever.hpp>
#include <libnpos/dev/msg/Open.hpp>
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
                    return read(request);    
                else if(request.getOperation() == msg::Transciever::Operation::WRITE)
                    return write(request);
            }
            else if(message.type == msg::Type::OPEN)
                open(message);
        }

        bool accepts(ipc::Message::Type type) const override
        {
            return (type == msg::Type::TRANSCIEVER or 
                    type == msg::Type::OPEN);
        }

    protected:
        void read(msg::Transciever& request)
        {
            auto buffer = request.getOutputBuffer();
            auto result = uart.receive(buffer.data(), request.getLength());
            auto status = (result == true ? msg::Transciever::Status::SUCCESS : msg::Transciever::Status::DEVICE_ERROR);

            getApi().respond(request.setStatus(status));
        }

        void write(msg::Transciever& request)
        {
            auto buffer = request.getInputBuffer();
            auto result = uart.transmit(buffer.data(), buffer.size());
            auto status = (result == true ? msg::Transciever::Status::SUCCESS : msg::Transciever::Status::DEVICE_ERROR);

            getApi().respond(request.setStatus(status));
        }

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
        hal::UartPolling& uart;
        etl::string_view name;
        bool isInitalized = false;
    };