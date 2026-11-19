#pragma once

/**
 * @file Sha256sum.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <etl/span.h>
#include <etl/string.h>

#include <libnpos/mdlwr/cli/utils/Command.hpp>
#include <libnpos/mdlwr/dev/Api.hpp>
#include <libnpos/dev/hal/Hash.hpp>

namespace npos::mdlwr::cli::program
{
    // sha256sum <file>... - print the SHA-256 digest and the name of every file, like `sha256sum` does.
    // The digest is counted by the dev::service::FileHash service.
    // The files are handled one after another, a failing one is reported and does not stop the rest.
    class Sha256sum : public utils::Command
    {
    public:
        explicit Sha256sum(Api& api)
            : utils::Command(api, "sha256sum", "sha256sum <file>... - print SHA-256 digests of files",
                "usage: sha256sum <file>...\r\n", "", 1)
        {
        }

        void resume() override
        {
            if(state == State::PRINTING)
                print();
        }

        void handleResponse(npos::ipc::Message& message) override
        {
            if(state != State::HASHING or message.type != mdlwr::dev::msg::Type::FILE_HASH)
                return;

            auto status = static_cast<mdlwr::dev::msg::file::FileHash&>(message).getStatus();

            if(status != mdlwr::dev::msg::file::FileHash::SUCCESS)
            {
                error("cannot read", operand, {}, utils::text::describe(status));
                return next();
            }

            line.clear();

            for(auto byte : digest)
                npos::cli::utils::text::appendHex(line, byte);

            line.append("  ");
            line.append(operand.data(), operand.size());
            line.append("\r\n");
            offset = 0;
            state = State::PRINTING;
            print();
        }

    protected:
        void begin() override
        {
            operandsLeft = operands;
            next();
        }

    private:
        enum class State
        {
            IDLE,
            HASHING,
            PRINTING
        };

        void next()
        {
            while(operandsLeft > 0)
            {
                operandsLeft--;
                operand = nextOperand();

                if(resolve(operand, path))
                {
                    state = State::HASHING;
                    return mdlwr::dev::api::file::hash(*this, etl::string_view(path.c_str(), path.size()),
                        etl::span<uint8_t>(digest, sizeof(digest)));
                }

                error("cannot read", operand, {}, "name too long");
            }

            state = State::IDLE;
            finish();
        }

        void print()
        {
            offset += host->print(etl::string_view(line.c_str() + offset, line.size() - offset));

            if(offset >= line.size())
                next();
        }

        State state = State::IDLE;
        size_t operandsLeft = 0;
        etl::string_view operand;
        Path path;
        uint8_t digest[npos::dev::hal::Hash::DIGEST_SIZE] = {};
        etl::string<2 * npos::dev::hal::Hash::DIGEST_SIZE + 2 + ARGUMENTS_SIZE + 2> line;
        size_t offset = 0;
    };
}
