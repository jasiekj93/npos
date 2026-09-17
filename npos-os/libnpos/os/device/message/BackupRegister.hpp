#pragma once

/**
 * @file BackupRegister.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <etl/optional.hpp>

#include <libnpos/os/message/Id.hpp>
#include <libnpos/os/message/RequestResponse.hpp>

namespace npos::os::device::message
{
    struct BackupRegister
    {
        using Register = uint8_t;
        using Value = uint32_t;

        struct Request : public os::message::Request
        {
            static constexpr auto ID = device::message::Id::BACKUP_REGISTER_REQUEST;

            Register reg;
            /**
             * @brief Set value for SET operation, ignored for GET operation.
             * 
             */
            etl::optional<Value> value;

            Request(Request::SenderId senderId, Register reg, etl::optional<Value> value = etl::nullopt)
                : os::message::Request(senderId)
                , reg(reg)
                , value(value)
            {
            }
        };

        struct Response : public os::message::Response
        {
            static constexpr auto ID = device::message::Id::BACKUP_REGISTER_RESPONSE;

            Register reg;
            etl::optional<Value> value;

            Response(Register reg, etl::optional<Value> value = etl::nullopt)
                : os::message::Response()
                , reg(reg)
                , value(value)
            {
            }
        };
    };
}