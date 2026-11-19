#pragma once

/**
 * @file Copy.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>

#include <libnpos/mdlwr/fs/msg/Type.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::msg::dir
{
    struct Copy : public ipc::Message
    {
        Copy(etl::string_view oldPath, etl::string_view newPath)
        {
            this->type = msg::Type::DIR_COPY;
            this->input.data = reinterpret_cast<const uint8_t*>(oldPath.data());
            this->input.size = oldPath.size();
            this->output.data = reinterpret_cast<uint8_t*>(const_cast<char*>(newPath.data()));
            this->output.size = newPath.size();
        }

        etl::string_view getOldPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        etl::string_view getNewPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->output.data), this->output.size);
        }

        msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        Copy& setStatus(msg::Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}