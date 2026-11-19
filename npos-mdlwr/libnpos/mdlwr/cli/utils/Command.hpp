#pragma once

/**
 * @file Command.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>
#include <etl/string_view.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>
#include <libnpos/fs/Info.hpp>
#include <libnpos/mdlwr/cli/utils/Text.hpp>

namespace npos::mdlwr::cli::utils
{
    // Base of the programs which work like their linux namesakes: `name [-options] operand...`.
    // It parses the options and counts the operands, a derived program takes it from there in begin().
    // Operands are views into a copy of the arguments, so they stay valid until the next start().
    //
    // The programs send their requests without a recipient, so the services of the middleware are found
    // by the type of the request. They have to run in another task than the terminal.
    class Command : public npos::cli::Program
    {
    public:
        static constexpr size_t ARGUMENTS_SIZE = 128;

        using Path = etl::string<npos::cli::utils::text::MAX_PATH>;

        void start(Host& terminal, etl::string_view arguments) override
        {
            host = &terminal;
            exitCode = 0;
            recursive = force = false;

            if(arguments.size() > ARGUMENTS_SIZE)
            {
                exitCode = 1;
                host->print(getName());
                host->print(": arguments too long\r\n");
                return finish();
            }

            text.assign(arguments.data(), arguments.size());
            rest = etl::string_view(text.c_str(), text.size());

            if(not parseOptions())
                return finish();

            countOperands();

            if(operands < minOperands)
            {
                exitCode = 1;
                host->print(usage);
                return finish();
            }

            begin();
        }

    protected:
        // `options` lists the accepted ones: 'r' stands for -r and -R, 'f' for -f.
        Command(Api& api, etl::string_view name, etl::string_view description, etl::string_view usage,
            etl::string_view options, size_t minOperands)
            : Program(api, name, description)
            , usage(usage)
            , options(options)
            , minOperands(minOperands)
        {
        }

        // The options are parsed and there are at least as many operands as the program needs.
        virtual void begin() = 0;

        void finish()
        {
            host->exit(*this, exitCode);
        }

        // Cuts the next operand off the arguments, empty when there are no more.
        etl::string_view nextOperand()
        {
            return npos::cli::utils::text::token(rest);
        }

        bool resolve(etl::string_view operand, Path& path)
        {
            return npos::cli::utils::text::resolve(host->getWorkingDirectory(), operand, path);
        }

        void stat(const Path& path)
        {
            npos::fs::api::stat(*this, etl::string_view(path.c_str(), path.size()), info);
        }

        // Prints `name: action 'first'[ to 'second']: reason` and remembers that the program failed.
        void error(const char* action, etl::string_view first, etl::string_view second, const char* reason)
        {
            exitCode = 1;
            host->print(getName());
            host->print(": ");
            host->print(action);
            host->print(" '");
            host->print(first);
            host->print("'");

            if(not second.empty())
            {
                host->print(" to '");
                host->print(second);
                host->print("'");
            }

            host->print(": ");
            host->print(reason);
            host->print("\r\n");
        }

        Host* host = nullptr;
        int exitCode = 0;
        bool recursive = false;
        bool force = false;
        size_t operands = 0;
        etl::string_view lastOperand;
        npos::fs::Info info;

    private:
        // Leading words starting with '-' are options, "--" ends them.
        bool parseOptions()
        {
            while(true)
            {
                auto next = rest;
                auto word = npos::cli::utils::text::token(next);

                if(word.size() < 2 or word.front() != '-')
                    return true;

                rest = next;

                if(word == etl::string_view("--"))
                    return true;

                for(auto letter : word.substr(1))
                {
                    if(not setOption(letter))
                    {
                        exitCode = 1;
                        host->print(getName());
                        host->print(": invalid option -- '");
                        host->print(etl::string_view(&letter, 1));
                        host->print("'\r\n");
                        host->print(usage);
                        return false;
                    }
                }
            }
        }

        bool setOption(char letter)
        {
            if((letter == 'r' or letter == 'R') and options.find('r') != etl::string_view::npos)
                recursive = true;
            else if(letter == 'f' and options.find('f') != etl::string_view::npos)
                force = true;
            else
                return false;

            return true;
        }

        void countOperands()
        {
            auto probe = rest;
            operands = 0;

            for(auto word = npos::cli::utils::text::token(probe); not word.empty();
                word = npos::cli::utils::text::token(probe))
            {
                operands++;
                lastOperand = word;
            }
        }

        etl::string_view usage;
        etl::string_view options;
        size_t minOperands;
        etl::string<ARGUMENTS_SIZE> text;
        etl::string_view rest;
    };
}
