#pragma once

/**
 * @file Write.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>
#include <etl/span.h>

#include <libnpos/mdlwr/dev/msg/Type.hpp>
#include <libnpos/mdlwr/dev/msg/Status.hpp>

namespace npos::mdlwr::dev::msg::file
{
    struct Write : public npos::ipc::Message
    {
        Write(npos::ipc::Oid recipient, etl::string_view pathToFile, etl::span<const uint8_t> input)
        {
            this->type = msg::Type::FILE_WRITE;
            this->recipient = recipient;
            this->input.data = input.data();
            this->input.size = input.size();
            this->output.data = reinterpret_cast<uint8_t*>(const_cast<char*>(pathToFile.data()));
            this->output.size = pathToFile.size();
        }

        etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->output.data), this->output.size);
        }

        etl::span<const uint8_t> getInput() const
        {
            return etl::span<const uint8_t>(this->input.data, this->input.size);
        }

        Status getStatus() const
        {
            return static_cast<Status>(this->output.status);
        }

        Write& setStatus(Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}