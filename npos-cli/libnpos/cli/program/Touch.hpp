#pragma once

/**
 * @file Touch.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    class Touch : public Program
    {
    public:
        explicit Touch(Api& api)
            : Program(api, "touch", "touch <file> - create an empty file")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            auto rest = args;
            auto name = text::token(rest);

            if(name.empty())
            {
                host->print("usage: touch <file>\r\n");
                return host->exit(*this, 1);
            }

            if(not text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("touch: path too long\r\n");
                return host->exit(*this, 1);
            }

            exitCode = 0;
            state = State::OPENING;
            fs::api::file::open(*this, etl::string_view(path.c_str(), path.size()), fs::Flags::WRITE_ONLY | fs::Flags::CREATE);
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == fs::msg::Type::FILE_OPEN and state == State::OPENING)
            {
                auto& response = static_cast<fs::msg::file::Open&>(message);

                if(response.getStatus() != fs::msg::Status::SUCCESS)
                {
                    report(response.getStatus());
                    state = State::IDLE;
                    return host->exit(*this, exitCode);
                }

                state = State::CLOSING;
                fs::api::file::close(*this, response.getOpened());
            }
            else if(message.type == fs::msg::Type::FILE_CLOSE and state == State::CLOSING)
            {
                state = State::IDLE;
                host->exit(*this, exitCode);
            }
        }

    private:
        enum class State { IDLE, OPENING, CLOSING };

        void report(fs::msg::Status status)
        {
            exitCode = 1;
            host->print("touch: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(text::describe(status));
            host->print("\r\n");
        }

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<text::MAX_PATH> path;
        int exitCode = 0;
    };
}
