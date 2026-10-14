#pragma once

/**
 * @file Ls.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    class Ls : public Program
    {
    public:
        explicit Ls(Api& api)
            : Program(api, "ls", "ls [dir] - list directory")
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            auto rest = args;
            auto name = text::token(rest);

            if(not text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("ls: path too long\r\n");
                return host->exit(*this, 1);
            }

            exitCode = 0;
            line.clear();
            offset = 0;
            state = State::OPENING;
            fs::api::dir::open(*this, etl::string_view(path.c_str(), path.size()));
        }

        void resume() override
        {
            if(state == State::PRINTING)
                print();
        }

        void handle(ipc::Message& message) override
        {
            switch(message.type)
            {
            case fs::msg::Type::DIR_OPEN:
                return onOpen(static_cast<fs::msg::dir::Open&>(message));
            case fs::msg::Type::DIR_READ:
                return onRead(static_cast<fs::msg::dir::Read&>(message));
            case fs::msg::Type::DIR_CLOSE:
                return onClose();
            default:
                return;
            }
        }

    private:
        enum class State { IDLE, OPENING, READING, PRINTING, CLOSING };

        void onOpen(fs::msg::dir::Open& response)
        {
            if(state != State::OPENING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
            {
                report(response.getStatus());
                state = State::IDLE;
                return host->exit(*this, exitCode);
            }

            dir = response.getOpened();
            readNext();
        }

        void onRead(fs::msg::dir::Read& response)
        {
            if(state != State::READING)
                return;

            auto status = response.getStatus();

            if(status == fs::msg::Status::END_OF_DIRECTORY)
                return close();

            if(status != fs::msg::Status::SUCCESS)
            {
                report(status);
                return close();
            }

            line.clear();
            offset = 0;

            if(entry.type == fs::msg::dir::Entry::DIRECTORY)
                line.append("     <dir> ");
            else
                { text::appendNumber(line, entry.size, 10); line.push_back(' '); }

            auto name = entry.getName();
            line.append(name.data(), name.size());

            if(entry.type == fs::msg::dir::Entry::DIRECTORY)
                line.push_back('/');

            line.append("\r\n");
            state = State::PRINTING;
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
            offset += host->print(etl::string_view(line.c_str() + offset, line.size() - offset));

            if(offset >= line.size())
                readNext();
        }

        void readNext()
        {
            state = State::READING;
            fs::api::dir::read(*this, dir, entry);
        }

        void close()
        {
            state = State::CLOSING;
            fs::api::dir::close(*this, dir);
        }

        void report(fs::msg::Status status)
        {
            exitCode = 1;
            host->print("ls: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(text::describe(status));
            host->print("\r\n");
        }

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<text::MAX_PATH> path;
        ipc::Oid dir = ipc::NULL_OID;
        fs::msg::dir::Entry entry;
        etl::string<fs::msg::dir::Entry::NAME_SIZE + 16> line;
        size_t offset = 0;
        int exitCode = 0;
    };
}
