#pragma once

/**
 * @file Count.hpp
 * @author Adrian Szczepanski
 * @date 24-11-2026
 */

#include <etl/string.h>

#include <libnpos/mdlwr/fs/msg/Type.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::msg::dir
{
    struct Count : public ipc::Message
    {
        Count(etl::string_view path)
        {
            this->type = msg::Type::DIR_COUNT;
            this->input.data = reinterpret_cast<const uint8_t*>(path.data());
            this->input.size = path.size();
        }

        etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        Count& setStatus(msg::Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }

        uint32_t getCount() const
        {
            return static_cast<uint32_t>(this->output.attributes.value);
        }

        Count& setCount(uint32_t count)
        {
            this->output.attributes.value = static_cast<ipc::Message::Output::Attributes::Value>(count);
            return *this;
        }
    };
}