#pragma once

/**
 * @file Terminal.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/algorithm.h>
#include <etl/span.h>
#include <etl/string.h>
#include <etl/string_view.h>
#include <etl/vector.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/Text.hpp>
#include <libnpos/dev/Api.hpp>

namespace npos::cli::service
{
    // Line terminal over a dev::msg::Transciever device (e.g. uart). Runs registered programs.
    class Terminal : public ipc::Service, public Program::Host
    {
    public:
        static constexpr size_t LINE_SIZE = 128;
        static constexpr size_t OUTPUT_SIZE = 512;
        static constexpr size_t CHUNK_SIZE = 64;
        static constexpr size_t MAX_READ_ERRORS = 3;

        using ProgramListInt = etl::ivector<Program*>;

        template<size_t N>
        using ProgramList = etl::vector<Program*, N>;

        Terminal(Api& api, ProgramListInt& programs, etl::string_view deviceName)
            : ipc::Service(api)
            , programs(programs)
            , deviceName(deviceName)
        {
        }

        bool addProgram(Program& program)
        {
            if(programs.full())
                return false;

            programs.push_back(&program);
            return true;
        }

        void initialize() override
        {
            api.syslog(ipc::msg::Syslog::INFO, "terminal: opening device");
            dev::api::open(*this, deviceName);
        }

        void handle(ipc::Message& message) override
        {
            if(message.type == dev::msg::Type::OPEN)
                onOpen(static_cast<dev::msg::Open&>(message));
            else if(message.type == dev::msg::Type::TRANSCIEVER)
                onTransciever(static_cast<dev::msg::Transciever&>(message));
        }

        // Terminal only consumes responses.
        bool accepts(ipc::Message::Type) const override { return false; }

        // Program::Host
        size_t print(etl::string_view output) override
        {
            size_t accepted = 0;

            for(auto c : output)
            {
                bool expand = (c == '\n' and lastOutput != '\r');

                if(pending.available() < (expand ? 2u : 1u))
                    break;

                if(expand)
                    pending.push_back('\r');

                pending.push_back(static_cast<uint8_t>(c));
                lastOutput = c;
                accepted++;
            }

            flush();
            return accepted;
        }

        void exit(Program& program, int) override
        {
            if(running != &program)
                return;

            running = nullptr;
            prompt();
        }

    private:
        void onOpen(dev::msg::Open& response)
        {
            if(response.getStatus() != dev::msg::Open::Status::SUCCESS)
                return api.syslog(ipc::msg::Syslog::ERROR, "terminal: cannot open device");

            device = response.getOpened();
            opened = true;
            api.syslog(ipc::msg::Syslog::INFO, "terminal: device opened");

            print("\r\nnpos terminal, type 'help'\r\n");
            prompt();
            startRead();
        }

        void onTransciever(dev::msg::Transciever& response)
        {
            bool success = response.getStatus() == dev::msg::Transciever::SUCCESS;

            if(response.getOperation() == dev::msg::Transciever::READ)
            {
                if(success)
                {
                    readErrors = 0;
                    onByte(rxByte);
                    startRead();
                }
                else if(++readErrors < MAX_READ_ERRORS)
                    startRead();
                else
                    api.syslog(ipc::msg::Syslog::ERROR, "terminal: input lost");
            }
            else
            {
                writing = false;

                if(not success)
                    api.syslog(ipc::msg::Syslog::WARNING, "terminal: output write failed");

                flush();

                if(not writing and running)
                    running->resume();
            }
        }

        void onByte(uint8_t c)
        {
            if(running)
                return;

            bool skipLineFeed = (c == '\n' and lastInput == '\r');
            lastInput = c;

            if(skipLineFeed)
                return;

            if(c == '\r' or c == '\n')
            {
                print("\r\n");
                execute();
            }
            else if(c == 0x7f or c == 0x08)
            {
                if(not line.empty())
                {
                    line.pop_back();
                    print("\b \b");
                }
            }
            else if(c >= 0x20 and c < 0x7f and not line.full())
            {
                line.push_back(static_cast<char>(c));
                print(etl::string_view(&line.back(), 1));
            }
        }

        void execute()
        {
            auto rest = etl::string_view(line.c_str(), line.size());
            auto command = text::token(rest);

            if(command.empty())
                return prompt();

            if(command == "help")
                return help();

            if(command == "clear")
            {
                print("\x1b[2J\x1b[H");
                return prompt();
            }

            for(auto program : programs)
            {
                if(program->getName() == command)
                {
                    running = program;
                    return program->start(*this, rest);
                }
            }

            print(command);
            print(": command not found\r\n");
            prompt();
        }

        void help()
        {
            print("help - list commands\r\nclear - clear screen\r\n");

            for(auto program : programs)
            {
                print(program->getDescription());
                print("\r\n");
            }

            prompt();
        }

        void prompt()
        {
            line.clear();
            print("npos> ");
        }

        void startRead()
        {
            dev::api::transcieverRead(*this, device, 1, etl::span<uint8_t>(&rxByte, 1));
        }

        void flush()
        {
            if(not opened or writing or pending.empty())
                return;

            auto size = pending.size() < CHUNK_SIZE ? pending.size() : CHUNK_SIZE;
            etl::copy_n(pending.begin(), size, chunk);
            pending.erase(pending.begin(), pending.begin() + size);

            writing = true;
            dev::api::transcieverWrite(*this, device, etl::span<const uint8_t>(chunk, size));
        }

        ProgramListInt& programs;
        etl::string_view deviceName;
        ipc::Oid device = ipc::NULL_OID;
        bool opened = false;
        bool writing = false;
        size_t readErrors = 0;
        Program* running = nullptr;

        etl::string<LINE_SIZE> line;
        etl::vector<uint8_t, OUTPUT_SIZE> pending;
        uint8_t chunk[CHUNK_SIZE];
        uint8_t rxByte = 0;
        uint8_t lastInput = 0;
        char lastOutput = 0;
    };
}
