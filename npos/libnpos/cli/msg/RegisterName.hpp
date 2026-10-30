#pragma once

/**
 * @file RegisterName.hpp
 * @author Adrian Szczepanski
 * @date 30-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/cli/msg/Type.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::cli::msg
{
    struct RegisterName : public ipc::Message
    {
        enum Status : ipc::Message::Status
        {
            SUCCESS = 0,
            NO_MEMORY,
        };

        // The path must be null terminated.
        explicit RegisterName(ipc::Oid recipient, etl::string_view name)
        {
            this->type = msg::Type::REGISTER_NAME;
            this->recipient = recipient;
            this->input.data = reinterpret_cast<const uint8_t*>(name.data());
            this->input.size = name.size();
        }

        inline etl::string_view getName() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline RegisterName& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
