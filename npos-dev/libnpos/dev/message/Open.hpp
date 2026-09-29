#pragma once

/**
 * @file Open.hpp
 * @author Adrian Szczepanski
 * @date 29-09-2026
 */

#include <etl/string_view.h>

#include <libnpos/dev/message/Type.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::dev::message
{
    struct OpenRequest : public ipc::Message
    {
        enum Status : ipc::Message::Status
        {
            SUCCESS = 0x00,
            NOT_FOUND = 0x01,
            INIT_FAILURE = 0x02
        };

        OpenRequest(etl::string_view name)
        {
            this->type = message::Type::OPEN;
            this->input.data = reinterpret_cast<const uint8_t*>(name.data());
            this->input.size = name.size();
        }

        inline etl::string_view getName() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline ipc::Oid getOpened() const
        {
            return this->output.open.opened;
        }

        inline OpenRequest& setOpened(ipc::Oid opened)
        {
            this->output.open.opened = opened;
            return *this;
        }

        inline Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        inline OpenRequest& setStatus(Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}