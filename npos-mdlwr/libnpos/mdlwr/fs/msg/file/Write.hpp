#pragma once

/**
 * @file Write.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>
#include <etl/span.h>

#include <libnpos/mdlwr/fs/msg/Type.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::msg::file
{
    struct Write : public npos::ipc::Message
    {
        Write(etl::string_view pathToFile, etl::span<const uint8_t> input, bool append)
        {
            this->type = msg::Type::FILE_WRITE;
            this->input.data = input.data();
            this->input.size = input.size();
            this->input.readWrite.mode = append;
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
        
        bool getAppend() const
        {
            return this->input.readWrite.mode;
        }

        msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        Write& setStatus(msg::Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}