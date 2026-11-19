#pragma once

/**
 * @file Copy.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/span.h>

#include <libnpos/mdlwr/fs/msg/file/Copy.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::file
{
    // Opens both files and moves the data from one to the other through the buffer.
    // An existing destination is overwritten.
    class Copy : public Operation<msg::file::Copy>
    {
    public:
        Copy(Api& api, etl::span<uint8_t> buffer)
            : Operation(api, states, msg::Type::FILE_COPY)
            , buffer(buffer)
            , idle(*this)
            , openingSource(OPENING_SOURCE, *this, &Copy::onSourceOpened)
            , openingDestination(OPENING_DESTINATION, *this, &Copy::onDestinationOpened)
            , reading(READING, *this, &Copy::onRead)
            , writing(WRITING, *this, &Copy::onWritten)
            , closing(CLOSING, *this, &Copy::onClosed)
        {
            states[IDLE] = &idle;
            states[OPENING_SOURCE] = &openingSource;
            states[OPENING_DESTINATION] = &openingDestination;
            states[READING] = &reading;
            states[WRITING] = &writing;
            states[CLOSING] = &closing;
        }

    protected:
        ID begin(msg::file::Copy& request) override
        {
            if(not utils::assign(source, request.getOldPath()) or not utils::assign(destination, request.getNewPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);
            sourceFile = destinationFile = ipc::NULL_OID;
            result = msg::Status::SUCCESS;

            npos::fs::api::file::open(*this, utils::view(source), npos::fs::Flags::READ_ONLY);
            return OPENING_SOURCE;
        }

    private:
        enum : ID
        {
            OPENING_SOURCE = IDLE + 1,
            OPENING_DESTINATION,
            READING,
            WRITING,
            CLOSING,
            STATES
        };

        ID onSourceOpened(npos::fs::msg::file::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(utils::fromFsStatus(response.getStatus(), msg::Status::FILE_NOT_FOUND));

            sourceFile = response.getOpened();
            sourceFileSize = response.getSize();

            // Truncating the destination would destroy the source.
            if(source == destination)
                return fail(msg::Status::SUCCESS);

            npos::fs::api::file::open(*this, utils::view(destination),
                npos::fs::Flags::WRITE_ONLY | npos::fs::Flags::CREATE | npos::fs::Flags::TRUNCATE);
            return OPENING_DESTINATION;
        }

        ID onDestinationOpened(npos::fs::msg::file::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            destinationFile = response.getOpened();
            return readNext();
        }

        ID onRead(npos::fs::msg::file::Read& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            length = response.getOutputLength();

            if(length == 0)
                return fail(msg::Status::SUCCESS);

            npos::fs::api::file::write(*this, destinationFile, etl::span<const uint8_t>(buffer.data(), length));
            return WRITING;
        }

        ID onWritten(npos::fs::msg::file::Write& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            if(response.getOutputLength() < length)
                return fail(msg::Status::NO_SPACE_LEFT);

            return readNext();
        }

        ID onClosed(npos::fs::msg::file::Close& response)
        {
            if(result == msg::Status::SUCCESS and response.getStatus() != npos::fs::msg::Status::SUCCESS)
                result = utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR);

            return closeNext();
        }

        ID readNext()
        {
            npos::fs::api::file::read(*this, sourceFile, buffer.size(), buffer);
            return READING;
        }

        // Ends the operation with the status, but only after the opened files are closed.
        ID fail(msg::Status status)
        {
            result = status;
            return closeNext();
        }

        ID closeNext()
        {
            auto& handle = (destinationFile != ipc::NULL_OID) ? destinationFile : sourceFile;

            if(handle == ipc::NULL_OID)
                return finish(result, sourceFileSize);

            auto file = handle;
            handle = ipc::NULL_OID;
            npos::fs::api::file::close(*this, file);
            return CLOSING;
        }

        etl::span<uint8_t> buffer;
        utils::Path source;
        utils::Path destination;
        ipc::Oid sourceFile = ipc::NULL_OID;
        ipc::Oid destinationFile = ipc::NULL_OID;
        size_t sourceFileSize = 0;
        size_t length = 0;
        msg::Status result = msg::Status::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<Copy, npos::fs::msg::file::Open, npos::fs::msg::Type::FILE_OPEN> openingSource;
        Await<Copy, npos::fs::msg::file::Open, npos::fs::msg::Type::FILE_OPEN> openingDestination;
        Await<Copy, npos::fs::msg::file::Read, npos::fs::msg::Type::FILE_READ> reading;
        Await<Copy, npos::fs::msg::file::Write, npos::fs::msg::Type::FILE_WRITE> writing;
        Await<Copy, npos::fs::msg::file::Close, npos::fs::msg::Type::FILE_CLOSE> closing;
    };


    template<size_t BUFFER_SIZE>
    class CopyStatic : public Copy
    {
    public:
        CopyStatic(Api& api) 
            : Copy(api, staticBuffer) 
        {
        }

    private:
        etl::array<uint8_t, BUFFER_SIZE> staticBuffer;
    };
}
