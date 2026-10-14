#pragma once

/**
 * @file Echo.hpp
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
    // echo <text> prints the text, echo <text> > <file> or >> <file> writes it to a file.
    class Echo : public Program
    {
    public:
        explicit Echo(Api& api)
            : Program(api, "echo", "echo <text> [> | >> <file>] - print or write text")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            exitCode = 0;

            auto redirect = args.find('>');

            if(redirect == etl::string_view::npos)
            {
                host->print(utils::text::trim(args));
                host->print("\r\n");
                return host->exit(*this, 0);
            }

            auto body = utils::text::trim(args.substr(0, redirect));
            auto target = args.substr(redirect);
            bool append = target.size() > 1 and target[1] == '>';
            target.remove_prefix(append ? 2 : 1);
            auto name = utils::text::token(target);

            if(name.empty() or body.size() + 1 > data.capacity())
            {
                host->print("usage: echo <text> [> | >> <file>]\r\n");
                return host->exit(*this, 1);
            }

            if(not utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("echo: path too long\r\n");
                return host->exit(*this, 1);
            }

            data.assign(body.data(), body.size());
            data.push_back('\n');
            state = State::OPENING;

            fs::api::file::open(*this, etl::string_view(path.c_str(), path.size()),
                fs::Flags::WRITE_ONLY | fs::Flags::CREATE | (append ? fs::Flags::APPEND : fs::Flags::TRUNCATE));
        }

        void handleResponse(ipc::Message& message) override
        {
            switch(message.type)
            {
            case fs::msg::Type::FILE_OPEN:
                return onOpen(static_cast<fs::msg::file::Open&>(message));
            case fs::msg::Type::FILE_WRITE:
                return onWrite(static_cast<fs::msg::file::Write&>(message));
            case fs::msg::Type::FILE_CLOSE:
                return onClose();
            default:
                return;
            }
        }

    private:
        enum class State { IDLE, OPENING, WRITING, CLOSING };

        void onOpen(fs::msg::file::Open& response)
        {
            if(state != State::OPENING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
            {
                report(response.getStatus());
                state = State::IDLE;
                return host->exit(*this, exitCode);
            }

            file = response.getOpened();
            state = State::WRITING;
            fs::api::file::write(*this, file, etl::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.c_str()), data.size()));
        }

        void onWrite(fs::msg::file::Write& response)
        {
            if(state != State::WRITING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
                report(response.getStatus());

            state = State::CLOSING;
            fs::api::file::close(*this, file);
        }

        void onClose()
        {
            if(state == State::CLOSING)
            {
                state = State::IDLE;
                host->exit(*this, exitCode);
            }
        }

        void report(fs::msg::Status status)
        {
            exitCode = 1;
            host->print("echo: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(utils::text::describe(status));
            host->print("\r\n");
        }

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<utils::text::MAX_PATH> path;
        etl::string<96> data;
        ipc::Oid file = ipc::NULL_OID;
        int exitCode = 0;
    };
}
