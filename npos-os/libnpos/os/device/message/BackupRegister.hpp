#pragma once

/**
 * @file BackupRegister.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/optional.hpp>

#include <libnpos/os/device/message/Id.hpp>

namespace npos::os::device::message
{
    struct BackupRegister : public os::Message
    {
        static constexpr auto ID = device::message::Id::BACKUP_REGISTER;

        using Register = uint8_t;
        using Value = uint32_t;

        BackupRegister() 
        {
            this->type = ID;
        }
    };
}