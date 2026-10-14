#pragma once

/**
 * @file Cd.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    // The directory is opened first so a nonexistent path never becomes the working directory.
    class Cd : public Program
    {
    public:
        explicit Cd(Api& api)
            : Program(api, "cd", "cd [dir] - change working directory")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            auto rest = args;
            auto name = utils::text::token(rest);

            if(name.empty())
                name = "/";

            if(not utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("cd: path too long\r\n");
                return host->exit(*this, 1);
            }

            exitCode = 0;
            state = State::OPENING;
            fs::api::dir::open(*this, etl::string_view(path.c_str(), path.size()));
        }

        void handleResponse(ipc::Message& message) override
        {
            if(message.type == fs::msg::Type::DIR_OPEN and state == State::OPENING)
            {
                auto& response = static_cast<fs::msg::dir::Open&>(message);

                if(response.getStatus() != fs::msg::Status::SUCCESS)
                {
                    exitCode = 1;
                    host->print("cd: ");
                    host->print(etl::string_view(path.c_str(), path.size()));
                    host->print(": ");
                    host->print(utils::text::describe(response.getStatus()));
                    host->print("\r\n");
                    state = State::IDLE;
                    return host->exit(*this, exitCode);
                }

                host->setWorkingDirectory(etl::string_view(path.c_str(), path.size()));
                state = State::CLOSING;
                fs::api::dir::close(*this, response.getOpened());
            }
            else if(message.type == fs::msg::Type::DIR_CLOSE and state == State::CLOSING)
            {
                state = State::IDLE;
                host->exit(*this, exitCode);
            }
        }

    private:
        enum class State { IDLE, OPENING, CLOSING };

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<utils::text::MAX_PATH> path;
        int exitCode = 0;
    };
}
