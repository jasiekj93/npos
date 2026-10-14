#pragma once

/**
 * @file Pwd.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/cli/Program.hpp>

namespace npos::cli::program
{
    class Pwd : public Program
    {
    public:
        explicit Pwd(Api& api)
            : Program(api, "pwd", "pwd - print working directory")
        {
        }

        void start(Host& host, etl::string_view) override
        {
            host.print(host.getWorkingDirectory());
            host.print("\r\n");
            host.exit(*this, 0);
        }

        void handleResponse(ipc::Message&) override {}
    };
}
