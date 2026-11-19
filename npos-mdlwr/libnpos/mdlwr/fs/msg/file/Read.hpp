#pragma once

/**
 * @file Read.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>
#include <etl/span.h>

#include <libnpos/mdlwr/fs/msg/Type.hpp>
#include <libnpos/mdlwr/fs/msg/Status.hpp>

namespace npos::mdlwr::fs::msg::file
{
    struct Read : public npos::ipc::Message
    {
        Read(npos::ipc::Oid recipient, etl::string_view pathToFile, etl::span<uint8_t> output)
        {
            this->type = msg::Type::FILE_READ;
            this->recipient = recipient;
            this->input.data = reinterpret_cast<const uint8_t*>(pathToFile.data());
            this->input.size = pathToFile.size();
            this->output.data = output.data();
            this->output.size = output.size();
        }

        etl::string_view getPath() const
        {
            return etl::string_view(reinterpret_cast<const char*>(this->input.data), this->input.size);
        }

        etl::span<uint8_t> getOutput() const
        {
            return etl::span<uint8_t>(this->output.data, this->output.size);
        }

        ipc::Message::Output::ReadWrite::Length getOutputLength() const
        {
            return this->output.readWrite.length;
        }

        Read& setOutputLength(ipc::Message::Output::ReadWrite::Length length)
        {
            this->output.readWrite.length = length;
            return *this;
        }

        msg::Status getStatus() const
        {
            return static_cast<msg::Status>(this->output.status);
        }

        Read& setStatus(msg::Status status)
        {
            this->output.status = static_cast<npos::ipc::Message::Status>(status);
            return *this;
        }
    };
}