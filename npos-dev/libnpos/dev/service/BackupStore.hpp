#pragma once

/**
 * @file BackupStore.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Service.hpp>
#include <libnpos/dev/msg/BackupRegisters.hpp>
#include <libnpos/dev/msg/Open.hpp>
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
        {
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == msg::Type::BACKUP_REGISTERS)
            {
                auto& request = static_cast<msg::BackupRegisters&>(message);

                if(not isInitalized)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::NOT_INITIALIZED));

                if(request.getValue() == nullptr)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::INVALID_VALUE_PTR));

                if(request.getRegister() >= hal::BackupRegisters::COUNT)
                    return getApi().respond(request.setStatus(msg::BackupRegisters::Status::INVALID_REGISTER));

                if(request.getOperation() == msg::BackupRegisters::Operation::READ)
                    backupRead(request);
                else
                    backupWrite(request);
            }
            else if (message.type == msg::Type::OPEN)
                open(message);
        }

        
        bool accepts(ipc::Message::Type type) const override
        {
            return (type == msg::Type::BACKUP_REGISTERS or 
                    type == msg::Type::OPEN);
        }

    protected:
        void backupRead(device::msg::BackupRegisters& request)
        {
            request.setValue(backupRegisters.read(request.getRegister()));
            getApi().respond(request.setStatus(msg::BackupRegisters::Status::SUCCESS));
        }

        void backupWrite(device::msg::BackupRegisters& request)
        {
            backupRegisters.write(request.getRegister(), *request.getValue());
            getApi().respond(request.setStatus(msg::BackupRegisters::Status::SUCCESS));
        }

        void open(ipc::Message& message)
        {
            auto& request = static_cast<msg::Open&>(message);

            if(request.getName() != name)
                return;

            if(not isInitalized)
            {
                backupRegisters.enableAccess();
                isInitalized = true;
            }

            auto status = msg::Open::Status::SUCCESS;
            api.respond(request.setStatus(status).setOpened(getOid()));
        }

    private:
        hal::BackupRegisters& backupRegisters;
        etl::string_view name;
        bool isInitalized = false;
    };
}