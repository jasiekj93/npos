#pragma once

/**
 * @file Read.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/span.h>

#include <libnpos/mdlwr/fs/msg/file/Read.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::file
{
    // Reads the whole file into the output buffer of the request, BUFFER_TOO_SMALL when it does not fit.
    class Read : public Operation<msg::file::Read>
    {
    public:
        explicit Read(Api& api)
            : Operation(api, states, msg::Type::FILE_READ)
            , idle(*this)
            , opening(OPENING, *this, &Read::onOpened)
            , reading(READING, *this, &Read::onRead)
            , closing(CLOSING, *this, &Read::onClosed)
        {
            states[IDLE] = &idle;
            states[OPENING] = &opening;
            states[READING] = &reading;
            states[CLOSING] = &closing;
        }

    protected:
        ID begin(msg::file::Read& request) override
        {
            if(not utils::assign(path, request.getPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);
            output = request.getOutput();
            file = ipc::NULL_OID;
            size = total = 0;
            result = msg::Status::SUCCESS;

            npos::fs::api::file::open(*this, utils::view(path), npos::fs::Flags::READ_ONLY);
            return OPENING;
        }

    private:
        enum : ID
        {
            OPENING = IDLE + 1,
            READING,
            CLOSING,
            STATES
        };

        ID onOpened(npos::fs::msg::file::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return respond(utils::fromFsStatus(response.getStatus(), msg::Status::FILE_NOT_FOUND));

            file = response.getOpened();
            size = response.getSize();

            if(size > output.size())
                return close(msg::Status::BUFFER_TOO_SMALL);

            return (size == 0) ? close(msg::Status::SUCCESS) : readNext();
        }

        ID onRead(npos::fs::msg::file::Read& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return close(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            total += response.getOutputLength();

            // A file that got shorter in the meantime ends the reading early.
            if(response.getOutputLength() == 0 or total >= size)
                return close(msg::Status::SUCCESS);

            return readNext();
        }

        ID onClosed(npos::fs::msg::file::Close& response)
        {
            if(result == msg::Status::SUCCESS and response.getStatus() != npos::fs::msg::Status::SUCCESS)
                result = utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR);

            return respond(result);
        }

        ID readNext()
        {
            auto remaining = size - total;
            npos::fs::api::file::read(*this, file, remaining, etl::span<uint8_t>(output.data() + total, remaining));
            return READING;
        }

        ID close(msg::Status status)
        {
            result = status;
            npos::fs::api::file::close(*this, file);
            return CLOSING;
        }

        ID respond(msg::Status status)
        {
            if(auto* request = current.release())
                getApi().respond(request->setOutputLength(status == msg::Status::SUCCESS ? total : 0).setStatus(status));

            return IDLE;
        }

        utils::Path path;
        etl::span<uint8_t> output;
        ipc::Oid file = ipc::NULL_OID;
        size_t size = 0;
        size_t total = 0;
        msg::Status result = msg::Status::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<Read, npos::fs::msg::file::Open, npos::fs::msg::Type::FILE_OPEN> opening;
        Await<Read, npos::fs::msg::file::Read, npos::fs::msg::Type::FILE_READ> reading;
        Await<Read, npos::fs::msg::file::Close, npos::fs::msg::Type::FILE_CLOSE> closing;
    };
}
