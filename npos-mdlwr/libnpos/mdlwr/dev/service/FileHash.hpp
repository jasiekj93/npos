#pragma once

/**
 * @file FileHash.hpp
 * @author Adrian Szczepanski
 * @date 19-11-2026
 */

#include <etl/span.h>
#include <etl/string_view.h>

#include <libnpos/dev/Api.hpp>
#include <libnpos/dev/hal/Hash.hpp>
#include <libnpos/dev/msg/Timer.hpp>
#include <libnpos/fs/Api.hpp>
#include <libnpos/mdlwr/dev/msg/FileHash.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>

namespace npos::mdlwr::dev::service
{
    // Counts the hash of a file with the hash device of the given name and writes it to the output buffer
    // of the request, which has to take at least a digest.
    // The file is read in chunks of the size of the buffer given to the service. The device is opened for
    // the time of the operation only and both the file and the device are closed before the answer.
    // Nobody answers an open request of a device which does not exist, so a timer limits the wait for it.
    class FileHash : public fs::service::Operation<msg::file::FileHash, msg::file::FileHash::Status>
    {
    public:
        using Request = msg::file::FileHash;

        static constexpr npos::dev::Tick HASH_OPEN_TIMEOUT = 1000;

        FileHash(Api& api, etl::string_view hashName, etl::span<uint8_t> buffer)
            : Operation(api, states, msg::Type::FILE_HASH)
            , hashName(hashName)
            , buffer(buffer)
            , idle(*this)
            , openingFile(OPENING_FILE, *this, &FileHash::onFileOpened)
            , openingHash(OPENING_HASH, *this, &FileHash::onHashOpened)
            , reading(READING, *this, &FileHash::onRead)
            , hashing(HASHING, *this, &FileHash::onHashed)
            , closingFile(CLOSING_FILE, *this, &FileHash::onFileClosed)
            , closingHash(CLOSING_HASH, *this, &FileHash::onHashClosed)
        {
            states[IDLE] = &idle;
            states[OPENING_FILE] = &openingFile;
            states[OPENING_HASH] = &openingHash;
            states[READING] = &reading;
            states[HASHING] = &hashing;
            states[CLOSING_FILE] = &closingFile;
            states[CLOSING_HASH] = &closingHash;
        }

        void handleResponse(ipc::Message& message) override
        {
            if(message.type != npos::dev::msg::Type::TIMER)
                return Operation::handleResponse(message);

            if(timers > 0)
                timers--;

            // A timer of an earlier request cannot be called off, only the newest one counts.
            auto* state = getCurrentState();

            // Without a free slot the timer is not set at all, the wait is then not limited.
            auto fired = static_cast<npos::dev::msg::Timer&>(message).getStatus() == npos::dev::msg::Timer::SUCCESS;

            if(fired and timers == 0 and state != nullptr and state->getId() == OPENING_HASH)
                setState(close(Request::HASH_NOT_FOUND));
        }

    protected:
        ID begin(Request& request) override
        {
            if(request.getOutputBuffer().size() < npos::dev::hal::Hash::DIGEST_SIZE)
                return refuse(request, Request::BUFFER_TOO_SMALL);

            if(not fs::utils::assign(path, request.getPath()))
                return refuse(request, Request::INVALID_PATH);

            current.store(request);
            digest = request.getOutputBuffer().first(npos::dev::hal::Hash::DIGEST_SIZE);
            file = hash = ipc::NULL_OID;
            size = total = 0;
            lastChunk = false;
            result = Request::SUCCESS;

            npos::fs::api::file::open(*this, fs::utils::view(path), npos::fs::Flags::READ_ONLY);
            return OPENING_FILE;
        }

    private:
        enum : ID
        {
            OPENING_FILE = IDLE + 1,
            OPENING_HASH,
            READING,
            HASHING,
            CLOSING_FILE,
            CLOSING_HASH,
            STATES
        };

        ID onFileOpened(npos::fs::msg::file::Open& response)
        {
            switch(response.getStatus())
            {
            case npos::fs::msg::Status::SUCCESS:
                break;
            case npos::fs::msg::Status::NO_DIR_ENTRY:
                return finish(Request::FILE_NOT_FOUND);
            case npos::fs::msg::Status::IS_A_DIRECTORY:
                return finish(Request::IS_A_DIRECTORY);
            case npos::fs::msg::Status::NAME_TOO_LONG:
            case npos::fs::msg::Status::INVALID_PARAMETER:
                return finish(Request::INVALID_PATH);
            default:
                return finish(Request::DEVICE_FAILURE);
            }

            file = response.getOpened();
            size = response.getSize();

            npos::dev::api::open(*this, hashName);
            npos::dev::api::timerRegister(*this, npos::dev::msg::Timer::SINGLE_SHOT, HASH_OPEN_TIMEOUT);
            timers++;
            return OPENING_HASH;
        }

        ID onHashOpened(npos::dev::msg::Open& response)
        {
            switch(response.getStatus())
            {
            case npos::dev::msg::Open::SUCCESS:
                hash = response.getOpened();
                return readNext();
            case npos::dev::msg::Open::NOT_FOUND:
                return close(Request::HASH_NOT_FOUND);
            case npos::dev::msg::Open::LOCKED:
                return close(Request::HASH_BUSY);
            default:
                return close(Request::DEVICE_FAILURE);
            }
        }

        ID onRead(npos::fs::msg::file::Read& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return close(Request::DEVICE_FAILURE);

            auto length = response.getOutputLength();
            total += length;

            // A file that got shorter in the meantime ends with what was read.
            lastChunk = (length == 0 or total >= size);

            return accumulate(etl::span<const uint8_t>(buffer.data(), length));
        }

        ID onHashed(npos::dev::msg::Hash& response)
        {
            if(response.getStatus() != npos::dev::msg::Hash::SUCCESS)
                return close(Request::DEVICE_FAILURE);

            return lastChunk ? close(Request::SUCCESS) : readNext();
        }

        ID onFileClosed(npos::fs::msg::file::Close& response)
        {
            file = ipc::NULL_OID;

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                keepFailure(Request::DEVICE_FAILURE);

            return closeNext();
        }

        ID onHashClosed(npos::dev::msg::Close& response)
        {
            hash = ipc::NULL_OID;

            if(response.getStatus() != npos::dev::msg::Close::SUCCESS)
                keepFailure(Request::DEVICE_FAILURE);

            return closeNext();
        }

        ID readNext()
        {
            if(total >= size)
            {
                lastChunk = true;
                return accumulate({});
            }

            auto length = (size - total < buffer.size()) ? size - total : buffer.size();
            npos::fs::api::file::read(*this, file, length, etl::span<uint8_t>(buffer.data(), length));
            return READING;
        }

        ID accumulate(etl::span<const uint8_t> chunk)
        {
            if(lastChunk)
                npos::dev::api::hashAccumulateEnd(*this, hash, chunk, digest);
            else
                npos::dev::api::hashAccumulate(*this, hash, chunk);

            return HASHING;
        }

        // Ends the operation with the status, but only after the file and the device are closed.
        ID close(Request::Status status)
        {
            result = status;
            return closeNext();
        }

        ID closeNext()
        {
            if(file != ipc::NULL_OID)
            {
                npos::fs::api::file::close(*this, file);
                return CLOSING_FILE;
            }

            if(hash != ipc::NULL_OID)
            {
                npos::dev::api::close(*this, hash);
                return CLOSING_HASH;
            }

            return finish(result);
        }

        void keepFailure(Request::Status status)
        {
            if(result == Request::SUCCESS)
                result = status;
        }

        etl::string_view hashName;
        etl::span<uint8_t> buffer;
        etl::span<uint8_t> digest;
        fs::utils::Path path;
        ipc::Oid file = ipc::NULL_OID;
        ipc::Oid hash = ipc::NULL_OID;
        size_t size = 0;
        size_t total = 0;
        bool lastChunk = false;
        size_t timers = 0;
        Request::Status result = Request::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<FileHash, npos::fs::msg::file::Open, npos::fs::msg::Type::FILE_OPEN> openingFile;
        Await<FileHash, npos::dev::msg::Open, npos::dev::msg::Type::OPEN> openingHash;
        Await<FileHash, npos::fs::msg::file::Read, npos::fs::msg::Type::FILE_READ> reading;
        Await<FileHash, npos::dev::msg::Hash, npos::dev::msg::Type::HASH> hashing;
        Await<FileHash, npos::fs::msg::file::Close, npos::fs::msg::Type::FILE_CLOSE> closingFile;
        Await<FileHash, npos::dev::msg::Close, npos::dev::msg::Type::CLOSE> closingHash;
    };

    template<size_t BUFFER_SIZE>
    class FileHashStatic : public FileHash
    {
    public:
        FileHashStatic(Api& api, etl::string_view name)
            : FileHash(api, name, staticBuffer)
        {
        }

    private:
        etl::array<uint8_t, BUFFER_SIZE> staticBuffer;
    };
}
