#pragma once

/**
 * @file Rmdir.hpp
 * @author Adrian Szczepanski
 * @date 30-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    // rmdir <path> - remove an empty directory.
    class Rmdir : public npos::cli::Program
    {
    public:
        explicit Rmdir(Api& api)
            : Program(api, "rmdir", "rmdir <path> - remove an empty directory")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;

            auto rest = args;
            auto name = npos::cli::utils::text::token(rest);

            if(name.empty() or not npos::cli::utils::text::trim(rest).empty())
            {
                host->print("usage: rmdir <path>\r\n");
                return host->exit(*this, 1);
            }

            if(not npos::cli::utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("rmdir: path too long\r\n");
                return host->exit(*this, 1);
            }

            waiting = true;
            npos::fs::api::dir::remove(*this, etl::string_view(path.c_str(), path.size()));
        }

        void handleResponse(npos::ipc::Message& message) override
        {
            if(not waiting or message.type != npos::fs::msg::Type::DIR_REMOVE)
                return;

            waiting = false;
            auto status = static_cast<npos::fs::msg::dir::Remove&>(message).getStatus();

            if(status == npos::fs::msg::Status::SUCCESS)
                return host->exit(*this, 0);

            host->print("rmdir: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(npos::cli::utils::text::describe(status));
            host->print("\r\n");
            host->exit(*this, 1);
        }

    private:
        Host* host = nullptr;
        bool waiting = false;
        etl::string<npos::cli::utils::text::MAX_PATH> path;
    };
}
