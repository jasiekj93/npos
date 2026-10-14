#pragma once

/**
 * @file Cat.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/span.h>
#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    class Cat : public Program
    {
    public:
        explicit Cat(Api& api)
            : Program(api, "cat", "cat <file> - print file contents")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            auto rest = args;
            auto name = utils::text::token(rest);

            if(name.empty())
            {
                host->print("usage: cat <file>\r\n");
                return host->exit(*this, 1);
            }

            if(not utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("cat: path too long\r\n");
                return host->exit(*this, 1);
            }

            exitCode = 0;
            offset = length = 0;
            waiting = false;
            state = State::OPENING;
            fs::api::file::open(*this, etl::string_view(path.c_str(), path.size()), fs::Flags::READ_ONLY);
        }

        void resume() override
        {
            if(waiting)
                print();
        }

        void handle(ipc::Message& message) override
        {
            switch(message.type)
            {
            case fs::msg::Type::FILE_OPEN:
                return onOpen(static_cast<fs::msg::file::Open&>(message));
            case fs::msg::Type::FILE_READ:
                return onRead(static_cast<fs::msg::file::Read&>(message));
            case fs::msg::Type::FILE_CLOSE:
                return onClose();
            default:
                return;
            }
        }

    private:
        enum class State { IDLE, OPENING, READING, CLOSING };

        void onOpen(fs::msg::file::Open& response)
        {
            if(state != State::OPENING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
                return finish(response.getStatus());

            file = response.getOpened();
            state = State::READING;
            readNext();
        }

        void onRead(fs::msg::file::Read& response)
        {
            if(state != State::READING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
            {
                report(response.getStatus());
                return close();
            }

            length = response.getOutputLength();
            offset = 0;

            if(length == 0)
                return close();

            print();
        }

        void onClose()
        {
            if(state == State::CLOSING)
            {
                state = State::IDLE;
                host->exit(*this, exitCode);
            }
        }

        void print()
        {
            offset += host->print(etl::string_view(reinterpret_cast<const char*>(buffer) + offset, length - offset));

            waiting = offset < length;
            if(not waiting)
                readNext();
        }

        void readNext()
        {
            fs::api::file::read(*this, file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));
        }

        void close()
        {
            state = State::CLOSING;
            fs::api::file::close(*this, file);
        }

        // Used when the file could not be opened, so there is nothing to close.
        void finish(fs::msg::Status status)
        {
            report(status);
            state = State::IDLE;
            host->exit(*this, exitCode);
        }

        void report(fs::msg::Status status)
        {
            exitCode = 1;
            host->print("cat: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(utils::text::describe(status));
            host->print("\r\n");
        }

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<utils::text::MAX_PATH> path;
        ipc::Oid file = ipc::NULL_OID;
        uint8_t buffer[64];
        size_t offset = 0;
        size_t length = 0;
        bool waiting = false;
        int exitCode = 0;
    };
}
