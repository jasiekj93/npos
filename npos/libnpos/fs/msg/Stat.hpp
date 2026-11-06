#pragma once

/**
 * @file Stat.hpp
 * @author Adrian Szczepanski
 * @date 06-11-2026
 */

#include <cstdint>

#include <etl/string_view.h>

#include <libnpos/fs/msg/Type.hpp>
#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/fs/Info.hpp>
#include <libnpos/ipc/Message.hpp>

namespace npos::fs::msg
{
    struct Stat : public ipc::Message
    {
        Stat(etl::string_view path, Info& info)
        {
            this->type = msg::Type::STAT;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
            this->output.data = reinterpret_cast<uint8_t*>(&info);
            this->output.size = sizeof(Info);
        }

        inline etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        inline Info& getEntry() const
        {
            return *reinterpret_cast<Info*>(this->output.data);
        }

        inline msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        inline Stat& setStatus(msg::Status status)
        {
            this->output.status = static_cast<ipc::Message::Status>(status);
            return *this;
        }
    };
}
