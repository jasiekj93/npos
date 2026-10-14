#pragma once

/**
 * @file Mkdir.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    class Mkdir : public Program
    {
    public:
        explicit Mkdir(Api& api)
            : Program(api, "mkdir", "mkdir <dir> - create a directory")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            auto rest = args;
            auto name = utils::text::token(rest);

            if(name.empty())
            {
                host->print("usage: mkdir <dir>\r\n");
                return host->exit(*this, 1);
            }

            if(not utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("mkdir: path too long\r\n");
                return host->exit(*this, 1);
            }

            waiting = true;
            fs::api::dir::make(*this, etl::string_view(path.c_str(), path.size()));
        }

        void handleResponse(ipc::Message& message) override
        {
            if(message.type != fs::msg::Type::DIR_MAKE or not waiting)
                return;

            waiting = false;
            auto status = static_cast<fs::msg::dir::Make&>(message).getStatus();

            if(status == fs::msg::Status::SUCCESS)
                return host->exit(*this, 0);

            host->print("mkdir: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(utils::text::describe(status));
            host->print("\r\n");
            host->exit(*this, 1);
        }

    private:
        Host* host = nullptr;
        bool waiting = false;
        etl::string<utils::text::MAX_PATH> path;
    };
}
