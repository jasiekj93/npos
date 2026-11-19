#pragma once

/**
 * @file Transfer.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Command.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>

namespace npos::mdlwr::cli::utils
{
    // Base of cp and mv: `name [options] <source>... <destination>`.
    // The last operand is the destination. When it is an existing directory, every source goes into it
    // under its own name, otherwise there must be exactly one source and it becomes the destination.
    // The sources are handled one after another, a failing one is reported and does not stop the rest.
    class Transfer : public Command
    {
    public:
        void handleResponse(npos::ipc::Message& message) override
        {
            if(message.type == npos::fs::msg::Type::STAT)
            {
                auto status = static_cast<npos::fs::msg::Stat&>(message).getStatus();

                if(state == State::CHECKING_DESTINATION)
                    return onDestinationChecked(status);

                if(state == State::CHECKING_SOURCE)
                    return onSourceChecked(status);
            }
            else if(state == State::TRANSFERRING and message.type == requestType)
            {
                // Whatever the request, its status is kept in the same place.
                onTransferred(static_cast<mdlwr::fs::msg::Status>(message.output.status));
            }
        }

    protected:
        Transfer(Api& api, etl::string_view name, etl::string_view description, etl::string_view usage,
            etl::string_view options, const char* action)
            : Command(api, name, description, usage, options, 2)
            , action(action)
        {
        }

        // False when a directory is refused, which the program has to tell the user about itself.
        virtual bool directoriesAllowed() const = 0;

        // Sends the request moving the source to the target. Returns the type of the request, the response
        // of which finishes the transfer.
        virtual npos::ipc::Message::Type send(bool directory, etl::string_view source, etl::string_view target) = 0;

        void begin() override
        {
            sourcesLeft = operands - 1;

            if(not resolve(lastOperand, destination))
            {
                error("cannot stat", lastOperand, {}, "name too long");
                return finishTransfer();
            }

            state = State::CHECKING_DESTINATION;
            stat(destination);
        }

    private:
        enum class State
        {
            IDLE,
            CHECKING_DESTINATION,
            CHECKING_SOURCE,
            TRANSFERRING
        };

        void onDestinationChecked(npos::fs::msg::Status status)
        {
            if(status != npos::fs::msg::Status::SUCCESS and status != npos::fs::msg::Status::NO_DIR_ENTRY)
            {
                error("cannot stat", lastOperand, {}, npos::cli::utils::text::describe(status));
                return finishTransfer();
            }

            destinationExists = (status == npos::fs::msg::Status::SUCCESS);
            destinationIsDirectory = destinationExists and info.type == npos::fs::Info::DIRECTORY;

            if(sourcesLeft > 1 and not destinationIsDirectory)
            {
                error("target", lastOperand, {}, destinationExists ? "not a directory" :
                    npos::cli::utils::text::describe(npos::fs::msg::Status::NO_DIR_ENTRY));
                return finishTransfer();
            }

            nextSource();
        }

        void nextSource()
        {
            while(sourcesLeft > 0)
            {
                sourcesLeft--;
                sourceOperand = nextOperand();

                if(resolve(sourceOperand, source))
                {
                    state = State::CHECKING_SOURCE;
                    return stat(source);
                }

                error("cannot stat", sourceOperand, {}, "name too long");
            }

            finishTransfer();
        }

        void onSourceChecked(npos::fs::msg::Status status)
        {
            if(status != npos::fs::msg::Status::SUCCESS)
            {
                error("cannot stat", sourceOperand, {}, npos::cli::utils::text::describe(status));
                return nextSource();
            }

            auto directory = (info.type == npos::fs::Info::DIRECTORY);

            if(directory and not directoriesAllowed())
            {
                exitCode = 1;
                host->print(getName());
                host->print(": -r not specified; omitting directory '");
                host->print(sourceOperand);
                host->print("'\r\n");
                return nextSource();
            }

            if(not makeTarget())
            {
                error(action, sourceOperand, lastOperand, "invalid path");
                return nextSource();
            }

            requestType = send(directory, etl::string_view(source.c_str(), source.size()),
                etl::string_view(target.c_str(), target.size()));
            state = State::TRANSFERRING;
        }

        void onTransferred(mdlwr::fs::msg::Status status)
        {
            if(status != mdlwr::fs::msg::Status::SUCCESS)
                error(action, sourceOperand, lastOperand, npos::mdlwr::cli::utils::text::describe(status));

            nextSource();
        }

        // Into an existing directory the source keeps its name.
        bool makeTarget()
        {
            target = destination;

            if(not destinationIsDirectory)
                return true;

            auto slash = source.rfind('/');
            auto name = etl::string_view(source.c_str(), source.size()).substr(slash + 1);

            return not name.empty() and mdlwr::fs::utils::append(target, name);
        }

        void finishTransfer()
        {
            state = State::IDLE;
            finish();
        }

        const char* action;
        State state = State::IDLE;
        npos::ipc::Message::Type requestType = 0;
        size_t sourcesLeft = 0;
        etl::string_view sourceOperand;
        bool destinationExists = false;
        bool destinationIsDirectory = false;
        Path source;
        Path destination;
        Path target;
    };
}
