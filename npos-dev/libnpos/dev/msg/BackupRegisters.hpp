#pragma once

/**
 * @file BackupRegisters.hpp
 * @author Adrian Szczepanski
 * @date 02-10-2026
 */

#include <libnpos/ipc/Message.hpp>
#include <libnpos/dev/msg/Type.hpp>

namespace npos::dev::msg
{
    struct BackupRegisters : public ipc::Message
    {
        enum Operation : ipc::Message::Input::ReadWrite::Mode
        {
            READ,
            WRITE
        };

        enum Status : ipc::Message::Status
        {
            SUCCESS = 0,
            INVALID_REGISTER,
            INVALID_VALUE_PTR,
            NOT_INITIALIZED
        };

        using Value = uint32_t;
        using Register = uint8_t;

        BackupRegisters(ipc::Oid oid, Operation operation, Register reg, Value* value)
        {
            this->recipient = oid;
            this->type = msg::Type::BACKUP_REGISTERS;
            this->input.readWrite.mode = operation;
            this->input.readWrite.offset = reg;
            this->output.data = reinterpret_cast<uint8_t*>(value);
            this->output.size = sizeof(Value);
        }

        inline Operation getOperation() const
        {
            return static_cast<Operation>(this->input.readWrite.mode);
        }

        inline Register getRegister() const
        {
            return static_cast<Register>(this->input.readWrite.offset);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline BackupRegisters& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }

        inline Value* getValue() const
        {
            return reinterpret_cast<Value*>(this->output.data);
        }

        inline BackupRegisters& setValue(Value value)
        {
            if(this->output.data != nullptr)
                *reinterpret_cast<Value*>(this->output.data) = value;

            return *this;
        }
    };
}