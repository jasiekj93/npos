#pragma once

/**
 * @file FakeProgram.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>
#include <etl/vector.h>

#include <libnpos/cli/Program.hpp>

namespace mock
{
    class FakeProgram : public npos::cli::Program
    {
    public:
        FakeProgram(Api& api, etl::string_view name = "fake")
            : npos::cli::Program(api, name, "fake - test program")
        {
        }

        void start(Host& terminal, etl::string_view arguments) override
        {
            host = &terminal;
            args.assign(arguments.data(), arguments.size());
            started++;

            if(exitOnStart)
                terminal.exit(*this, 0);
        }

        void resume() override { resumed++; }
        void handleResponse(npos::ipc::Message& message) override { handled.push_back(message.type); }

        void finish(int code = 0) { host->exit(*this, code); }

        Host* host = nullptr;
        etl::string<64> args;
        int started = 0;
        int resumed = 0;
        bool exitOnStart = false;
        etl::vector<npos::ipc::Message::Type, 8> handled;
    };
}
