#pragma once

/**
 * @file Random.hpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <etl/random.h>
#include <etl/span.h>
#include <etl/string.h>

#include <libnpos/cli/Program.hpp>
#include <libnpos/cli/utils/Text.hpp>
#include <libnpos/fs/Api.hpp>

namespace npos::cli::program
{
    // random <size> prints random bytes as hex, random <size> > <file> or >> <file> writes them to a file.
    class Random : public Program
    {
    public:
        static constexpr uint32_t DEFAULT_SEED = 0x6e706f73;
        static constexpr size_t BYTES_PER_LINE = 32;

        // The generator keeps its state between runs, so every run gives other bytes.
        explicit Random(Api& api, uint32_t seed = DEFAULT_SEED)
            : Program(api, "random", "random <size> [> | >> <file>] - random bytes as hex or written to a file")
            , generator(seed)
        {
        }

        void start(Host& terminal, etl::string_view args) override
        {
            host = &terminal;
            exitCode = 0;
            offset = 0;
            waiting = false;
            line.clear();

            auto redirect = args.find('>');
            auto size = utils::text::trim(redirect == etl::string_view::npos ? args : args.substr(0, redirect));

            if(not utils::text::parseNumber(size, remaining))
            {
                host->print("usage: random <size> [> | >> <file>]\r\n");
                return host->exit(*this, 1);
            }

            if(redirect == etl::string_view::npos)
            {
                state = State::PRINTING;
                return printNext();
            }

            auto target = args.substr(redirect);
            bool append = target.size() > 1 and target[1] == '>';
            target.remove_prefix(append ? 2 : 1);
            auto name = utils::text::token(target);

            if(name.empty())
            {
                host->print("usage: random <size> [> | >> <file>]\r\n");
                return host->exit(*this, 1);
            }

            if(not utils::text::resolve(host->getWorkingDirectory(), name, path))
            {
                host->print("random: path too long\r\n");
                return host->exit(*this, 1);
            }

            state = State::OPENING;
            fs::api::file::open(*this, etl::string_view(path.c_str(), path.size()),
                fs::Flags::WRITE_ONLY | fs::Flags::CREATE | (append ? fs::Flags::APPEND : fs::Flags::TRUNCATE));
        }

        void resume() override
        {
            if(waiting and state == State::PRINTING)
                printNext();
        }

        void handleResponse(ipc::Message& message) override
        {
            switch(message.type)
            {
            case fs::msg::Type::FILE_OPEN:
                return onOpen(static_cast<fs::msg::file::Open&>(message));
            case fs::msg::Type::FILE_WRITE:
                return onWrite(static_cast<fs::msg::file::Write&>(message));
            case fs::msg::Type::FILE_CLOSE:
                return onClose();
            default:
                return;
            }
        }

    private:
        enum class State { IDLE, PRINTING, OPENING, WRITING, CLOSING };

        uint8_t nextByte()
        {
            if(bytesLeft == 0)
            {
                word = generator();
                bytesLeft = 4;
            }

            auto value = static_cast<uint8_t>(word & 0xff);
            word >>= 8;
            bytesLeft--;
            return value;
        }

        // Prints the bytes as hex lines; a line the host did not take completely is sent again by resume().
        void printNext()
        {
            while(true)
            {
                waiting = false;

                if(offset == line.size())
                {
                    if(remaining == 0)
                    {
                        state = State::IDLE;
                        return host->exit(*this, 0);
                    }

                    auto count = remaining < BYTES_PER_LINE ? remaining : BYTES_PER_LINE;
                    line.clear();
                    offset = 0;

                    for(size_t i = 0; i < count; i++)
                        utils::text::appendHex(line, nextByte());

                    line.append("\r\n");
                    remaining -= static_cast<uint32_t>(count);
                }

                offset += host->print(etl::string_view(line.data() + offset, line.size() - offset));

                if(offset < line.size())
                {
                    waiting = true;
                    return;
                }
            }
        }

        void onOpen(fs::msg::file::Open& response)
        {
            if(state != State::OPENING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
            {
                report(response.getStatus());
                state = State::IDLE;
                return host->exit(*this, exitCode);
            }

            file = response.getOpened();
            state = State::WRITING;
            writeNext();
        }

        void onWrite(fs::msg::file::Write& response)
        {
            if(state != State::WRITING)
                return;

            if(response.getStatus() != fs::msg::Status::SUCCESS)
            {
                report(response.getStatus());
                return close();
            }

            remaining -= static_cast<uint32_t>(chunk);
            writeNext();
        }

        void onClose()
        {
            if(state == State::CLOSING)
            {
                state = State::IDLE;
                host->exit(*this, exitCode);
            }
        }

        void writeNext()
        {
            if(remaining == 0)
                return close();

            chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);

            for(size_t i = 0; i < chunk; i++)
                buffer[i] = nextByte();

            fs::api::file::write(*this, file, etl::span<const uint8_t>(buffer, chunk));
        }

        void close()
        {
            state = State::CLOSING;
            fs::api::file::close(*this, file);
        }

        void report(fs::msg::Status status)
        {
            exitCode = 1;
            host->print("random: ");
            host->print(etl::string_view(path.c_str(), path.size()));
            host->print(": ");
            host->print(utils::text::describe(status));
            host->print("\r\n");
        }

        etl::random_xorshift generator;
        uint32_t word = 0;
        uint8_t bytesLeft = 0;

        Host* host = nullptr;
        State state = State::IDLE;
        etl::string<utils::text::MAX_PATH> path;
        ipc::Oid file = ipc::NULL_OID;
        uint8_t buffer[64];
        etl::string<BYTES_PER_LINE * 2 + 2> line;
        size_t offset = 0;
        size_t chunk = 0;
        uint32_t remaining = 0;
        bool waiting = false;
        int exitCode = 0;
    };
}
