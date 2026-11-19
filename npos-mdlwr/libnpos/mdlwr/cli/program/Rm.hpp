#pragma once

/**
 * @file Rm.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Command.hpp>
#include <libnpos/mdlwr/fs/Api.hpp>

namespace npos::mdlwr::cli::program
{
    // rm [-rf] <path>... - remove files. Directories only with -r (or -R), together with everything inside.
    // With -f a missing path is not an error.
    // The paths are handled one after another, a failing one is reported and does not stop the rest.
    class Rm : public utils::Command
    {
    public:
        explicit Rm(Api& api)
            : utils::Command(api, "rm", "rm [-rf] <path>... - remove files and directories",
                "usage: rm [-rf] <path>...\r\n", "rf", 1)
        {
        }

        void handleResponse(npos::ipc::Message& message) override
        {
            if(state == State::CHECKING and message.type == npos::fs::msg::Type::STAT)
                return onChecked(static_cast<npos::fs::msg::Stat&>(message).getStatus());

            if(state != State::REMOVING)
                return;

            if(message.type == npos::fs::msg::Type::FILE_REMOVE)
            {
                auto status = static_cast<npos::fs::msg::file::Remove&>(message).getStatus();
                onRemoved(npos::cli::utils::text::describe(status), status == npos::fs::msg::Status::SUCCESS);
            }
            else if(message.type == mdlwr::fs::msg::Type::DIR_REMOVE)
            {
                auto status = static_cast<mdlwr::fs::msg::dir::Remove&>(message).getStatus();
                onRemoved(npos::mdlwr::cli::utils::text::describe(status), status == mdlwr::fs::msg::Status::SUCCESS);
            }
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
            CHECKING,
            REMOVING
        };

        void next()
        {
            while(operandsLeft > 0)
            {
                operandsLeft--;
                operand = nextOperand();

                if(resolve(operand, path))
                {
                    state = State::CHECKING;
                    return stat(path);
                }

                error("cannot remove", operand, {}, "name too long");
            }

            state = State::IDLE;
            finish();
        }

        void onChecked(npos::fs::msg::Status status)
        {
            if(status != npos::fs::msg::Status::SUCCESS)
            {
                if(not (force and status == npos::fs::msg::Status::NO_DIR_ENTRY))
                    error("cannot remove", operand, {}, npos::cli::utils::text::describe(status));

                return next();
            }

            auto view = etl::string_view(path.c_str(), path.size());
            state = State::REMOVING;

            if(info.type != npos::fs::Info::DIRECTORY)
                return npos::fs::api::file::remove(*this, view);

            if(not recursive)
            {
                error("cannot remove", operand, {}, "is a directory");
                return next();
            }

            fs::api::dir::remove(*this, view);
        }

        void onRemoved(const char* reason, bool success)
        {
            if(not success)
                error("cannot remove", operand, {}, reason);

            next();
        }

        State state = State::IDLE;
        size_t operandsLeft = 0;
        etl::string_view operand;
        Path path;
    };
}
