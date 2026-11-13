#pragma once

/**
 * @file BackupStore.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/BackupRegisters.hpp>
#include <libnpos/dev/msg/Open.hpp>
#include <libnpos/dev/msg/Close.hpp>
#include <libnpos/dev/hal/BackupRegisters.hpp>

namespace npos::dev::service
{
    class BackupStore : public ipc::Service
    {
    public:
        BackupStore(Api& api, hal::BackupRegisters& backupRegisters, etl::string_view name)
            : ipc::Service(api)
            , backupRegisters(backupRegisters)
            , name(name)
            , owner(ipc::NULL_OID)
        {
        }

        void handleRequest(ipc::Message& message) override
        {
            if(message.type == msg::Type::BACKUP_REGISTERS)
            {
                auto& request = static_cast<msg::BackupRegisters&>(message);

                if(owner == ipc::NULL_OID)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::NOT_INITIALIZED));

                if(message.sender != owner)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::LOCKED));

                if(request.getValue() == nullptr)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::INVALID_VALUE_PTR));

                if(request.getRegister() >= hal::BackupRegisters::COUNT)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::INVALID_REGISTER));

                if(request.getOperation() == msg::BackupRegisters::Operation::READ)
                    backupRead(request);
                else if(request.getOperation() == msg::BackupRegisters::Operation::WRITE)
                    backupWrite(request);
                else
                    handleUnknownRequest(message);
            }
            else if (message.type == msg::Type::OPEN)
                open(message);
            else if (message.type == msg::Type::CLOSE)
                close(message);
        }

        void handleResponse(ipc::Message& message) override {}
        
        bool acceptsRequest(ipc::Message::Type type) const override
        {
            return (type == msg::Type::BACKUP_REGISTERS) or 
                    (type == msg::Type::OPEN) or
                    (type == msg::Type::CLOSE);
        }

    protected:
        void backupRead(msg::BackupRegisters& request)
        {
            request.setValue(backupRegisters.read(request.getRegister()));
            getApi().respond(request.setStatus(msg::BackupRegisters::Status::SUCCESS));
        }

        void backupWrite(msg::BackupRegisters& request)
        {
            backupRegisters.write(request.getRegister(), *request.getValue());
            getApi().respond(request.setStatus(msg::BackupRegisters::Status::SUCCESS));
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


            backupRegisters.enableAccess();
            owner = message.sender;

            auto status = msg::Open::Status::SUCCESS;
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
            backupRegisters.disableAccess();
            api.respond(request.setStatus(msg::Close::Status::SUCCESS));
        }

    private:
        hal::BackupRegisters& backupRegisters;
        etl::string_view name;
        ipc::Oid owner;
    };
}