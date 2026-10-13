#pragma once

/**
 * @file FakeHost.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>

namespace mock
{
    // Program::Host that records printed text and exits; its output can be limited to test back pressure.
    class FakeHost : public npos::cli::Program::Host
    {
    public:
        size_t print(etl::string_view text) override
        {
            size_t accepted = text.size() < capacity ? text.size() : capacity;
            capacity -= accepted;
            output.append(text.data(), accepted);
            return accepted;
        }

        void exit(npos::cli::Program& program, int code) override
        {
            exited = &program;
            exitCode = code;
            exitCount++;
        }

        etl::string_view getWorkingDirectory() const override
        {
            return etl::string_view(cwd.c_str(), cwd.size());
        }

        bool setWorkingDirectory(etl::string_view path) override
        {
            cwd.assign(path.data(), path.size());
            return true;
        }

        bool outputIs(const char* expected) const { return output == expected; }

        etl::string<2048> output;
        etl::string<64> cwd = "/";
        size_t capacity = 2048;
        npos::cli::Program* exited = nullptr;
        int exitCode = -1;
        int exitCount = 0;
    };
}
