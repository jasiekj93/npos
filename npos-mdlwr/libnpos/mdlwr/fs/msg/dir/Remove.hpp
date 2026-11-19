#pragma once

/**
 * @file Remove.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>

#include <libnpos/mdlwr/fs/msg/Type.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::msg::dir
{
    struct Remove : public ipc::Message
    {
        Remove(npos::ipc::Oid recipient, etl::string_view path)
        {
            this->type = msg::Type::DIR_REMOVE;
            this->recipient = recipient;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
        }

        etl::string_view getOldPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        Remove& setStatus(msg::Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}