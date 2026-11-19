#pragma once

/**
 * @file Write.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/span.h>

#include <libnpos/mdlwr/fs/msg/file/Write.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::file
{
    // Writes the whole input of the request to the file. The file is created when it does not exist,
    // an existing one is truncated or, for an appending request, written at the end.
    class Write : public Operation<msg::file::Write>
    {
    public:
        explicit Write(Api& api)
            : Operation(api, states, msg::Type::FILE_WRITE)
            , idle(*this)
            , opening(OPENING, *this, &Write::onOpened)
            , writing(WRITING, *this, &Write::onWritten)
            , closing(CLOSING, *this, &Write::onClosed)
        {
            states[IDLE] = &idle;
            states[OPENING] = &opening;
            states[WRITING] = &writing;
            states[CLOSING] = &closing;
        }

    protected:
        ID begin(msg::file::Write& request) override
        {
            if(not utils::assign(path, request.getPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);
            input = request.getInput();
            file = ipc::NULL_OID;
            written = 0;
            result = msg::Status::SUCCESS;

            npos::fs::api::file::open(*this, utils::view(path), npos::fs::Flags::WRITE_ONLY | npos::fs::Flags::CREATE |
                (request.getAppend() ? npos::fs::Flags::APPEND : npos::fs::Flags::TRUNCATE));
            return OPENING;
        }

    private:
        enum : ID
        {
            OPENING = IDLE + 1,
            WRITING,
            CLOSING,
            STATES
        };

        ID onOpened(npos::fs::msg::file::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            file = response.getOpened();
            return input.empty() ? close(msg::Status::SUCCESS) : writeNext();
        }

        ID onWritten(npos::fs::msg::file::Write& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return close(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            // The filesystem accepts nothing more, trying again would never end.
            if(response.getOutputLength() == 0)
                return close(msg::Status::NO_SPACE_LEFT);

            written += response.getOutputLength();

            return (written >= input.size()) ? close(msg::Status::SUCCESS) : writeNext();
        }

        ID onClosed(npos::fs::msg::file::Close& response)
        {
            if(result == msg::Status::SUCCESS and response.getStatus() != npos::fs::msg::Status::SUCCESS)
                result = utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR);

            return finish(result);
        }

        ID writeNext()
        {
            npos::fs::api::file::write(*this, file, etl::span<const uint8_t>(input.data() + written, input.size() - written));
            return WRITING;
        }

        ID close(msg::Status status)
        {
            result = status;
            npos::fs::api::file::close(*this, file);
            return CLOSING;
        }

        utils::Path path;
        etl::span<const uint8_t> input;
        ipc::Oid file = ipc::NULL_OID;
        size_t written = 0;
        msg::Status result = msg::Status::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<Write, npos::fs::msg::file::Open, npos::fs::msg::Type::FILE_OPEN> opening;
        Await<Write, npos::fs::msg::file::Write, npos::fs::msg::Type::FILE_WRITE> writing;
        Await<Write, npos::fs::msg::file::Close, npos::fs::msg::Type::FILE_CLOSE> closing;
    };
}
