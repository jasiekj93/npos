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
    struct BackupRegister
    {
        using Register = uint8_t;
        using Value = uint32_t;

        struct Request : public os::Message
        {
            static constexpr auto ID = device::message::Id::BACKUP_REGISTER_REQUEST;

            Register reg;
            /**
             * @brief Set value for SET operation, ignored for GET operation.
             * 
             */
            etl::optional<Value> value;

            Request() : os::Message(ID) {}
        };

        struct Response : public os::message::Response
        {
            static constexpr auto ID = device::message::Id::BACKUP_REGISTER_RESPONSE;

            Register reg;
            etl::optional<Value> value;

            Response() : os::Message(ID) {}
        };
    };
}