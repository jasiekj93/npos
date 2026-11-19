#pragma once

/**
 * @file Copy.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/array.h>

#include <libnpos/mdlwr/fs/msg/dir/Copy.hpp>
#include <libnpos/mdlwr/fs/msg/file/Copy.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::dir
{
    // Copies a directory with everything inside. The destination must not exist and the copy is not undone
    // when it fails halfway.
    // Files are copied by the file::Copy service, which receives msg::file::Copy requests from this one.
    // The requests are addressed to `fileCopy`, which has to be valid when the service is constructed
    // (its port and service id are already assigned). A broadcast would not do, because it never returns
    // to the port it comes from, so file::Copy in the same task would not get it.
    //
    // Only one directory is opened at a time. Before entering a subdirectory the current one is closed, so
    // the traversal remembers how many entries of each level it passed and skips them when it comes back.
    class Copy : public Operation<msg::dir::Copy>
    {
    public:
        static constexpr size_t MAX_DEPTH = 16;

        explicit Copy(Api& api, npos::ipc::Oid fileCopy)
            : Operation(api, states, msg::Type::DIR_COPY)
            , fileCopy(fileCopy)
            , idle(*this)
            , checkingSource(CHECKING_SOURCE, *this, &Copy::onSourceChecked)
            , checkingDestination(CHECKING_DESTINATION, *this, &Copy::onDestinationChecked)
            , making(MAKING, *this, &Copy::onMade)
            , opening(OPENING, *this, &Copy::onOpened)
            , reading(READING, *this, &Copy::onRead)
            , copying(COPYING, *this, &Copy::onFileCopied)
            , closing(CLOSING, *this, &Copy::onClosed)
        {
            states[IDLE] = &idle;
            states[CHECKING_SOURCE] = &checkingSource;
            states[CHECKING_DESTINATION] = &checkingDestination;
            states[MAKING] = &making;
            states[OPENING] = &opening;
            states[READING] = &reading;
            states[COPYING] = &copying;
            states[CLOSING] = &closing;
        }

    protected:
        ID begin(msg::dir::Copy& request) override
        {
            if(not utils::assign(source, request.getOldPath()) or not utils::assign(destination, request.getNewPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            // Copying into itself would never end.
            if(utils::isInside(utils::view(source), utils::view(destination)))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);
            directory = ipc::NULL_OID;
            depth = 0;
            position[0] = 0;
            result = msg::Status::SUCCESS;

            npos::fs::api::stat(*this, utils::view(source), info);
            return CHECKING_SOURCE;
        }

    private:
        enum : ID
        {
            CHECKING_SOURCE = IDLE + 1,
            CHECKING_DESTINATION,
            MAKING,
            OPENING,
            READING,
            COPYING,
            CLOSING,
            STATES
        };

        // What happens when the opened directory is closed.
        enum class After
        {
            ENTER_ENTRY,
            LEAVE,
            FAIL
        };

        ID onSourceChecked(npos::fs::msg::Stat& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            if(info.type != npos::fs::Info::DIRECTORY)
                return finish(msg::Status::NOT_A_DIRECTORY);

            npos::fs::api::stat(*this, utils::view(destination), info);
            return CHECKING_DESTINATION;
        }

        ID onDestinationChecked(npos::fs::msg::Stat& response)
        {
            if(response.getStatus() == npos::fs::msg::Status::SUCCESS)
                return finish(msg::Status::ALREADY_EXISTS);

            if(response.getStatus() != npos::fs::msg::Status::NO_DIR_ENTRY)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            return make();
        }

        ID onMade(npos::fs::msg::dir::Make& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            return open();
        }

        ID onOpened(npos::fs::msg::dir::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            directory = response.getOpened();
            skipped = 0;
            return readNext();
        }

        ID onRead(npos::fs::msg::dir::Read& response)
        {
            if(response.getStatus() == npos::fs::msg::Status::END_OF_DIRECTORY)
                return close(After::LEAVE);

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return fail(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            // Entries of this level that were already handled before the directory was reopened.
            if(skipped < position[depth])
            {
                skipped++;
                return readNext();
            }

            // Entries read from now on, until the directory is reopened, are all new.
            skipped = ++position[depth];
            sourceLength = source.size();
            destinationLength = destination.size();

            if(not utils::append(source, info.getName()) or not utils::append(destination, info.getName()))
                return fail(msg::Status::INVALID_PATH);

            if(info.type == npos::fs::Info::DIRECTORY)
            {
                if(depth + 1 >= MAX_DEPTH)
                    return fail(msg::Status::INVALID_PATH);

                return close(After::ENTER_ENTRY);
            }

            msg::file::Copy request(fileCopy, utils::view(source), utils::view(destination));
            getApi().send(*this, request);
            return COPYING;
        }

        ID onFileCopied(msg::file::Copy& response)
        {
            source.resize(sourceLength);
            destination.resize(destinationLength);

            if(response.getStatus() != msg::Status::SUCCESS)
                return fail(response.getStatus());

            return readNext();
        }

        ID onClosed(npos::fs::msg::dir::Close& response)
        {
            if(after == After::FAIL)
                return finish(result);

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            if(after == After::ENTER_ENTRY)
            {
                position[++depth] = 0;
                return make();
            }

            if(depth == 0)
                return finish(msg::Status::SUCCESS);

            depth--;
            utils::removeLast(source);
            utils::removeLast(destination);
            return open();
        }

        ID make()
        {
            npos::fs::api::dir::make(*this, utils::view(destination));
            return MAKING;
        }

        ID open()
        {
            npos::fs::api::dir::open(*this, utils::view(source));
            return OPENING;
        }

        ID readNext()
        {
            npos::fs::api::dir::read(*this, directory, info);
            return READING;
        }

        ID close(After next)
        {
            after = next;
            npos::fs::api::dir::close(*this, directory);
            directory = ipc::NULL_OID;
            return CLOSING;
        }

        // Ends the operation with the status, but only after the opened directory is closed.
        ID fail(msg::Status status)
        {
            result = status;
            return close(After::FAIL);
        }

        utils::Path source;
        utils::Path destination;
        npos::fs::Info info;
        ipc::Oid fileCopy = ipc::NULL_OID;
        ipc::Oid directory = ipc::NULL_OID;
        etl::array<uint16_t, MAX_DEPTH> position = {};
        size_t depth = 0;
        size_t skipped = 0;
        size_t sourceLength = 0;
        size_t destinationLength = 0;
        After after = After::FAIL;
        msg::Status result = msg::Status::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<Copy, npos::fs::msg::Stat, npos::fs::msg::Type::STAT> checkingSource;
        Await<Copy, npos::fs::msg::Stat, npos::fs::msg::Type::STAT> checkingDestination;
        Await<Copy, npos::fs::msg::dir::Make, npos::fs::msg::Type::DIR_MAKE> making;
        Await<Copy, npos::fs::msg::dir::Open, npos::fs::msg::Type::DIR_OPEN> opening;
        Await<Copy, npos::fs::msg::dir::Read, npos::fs::msg::Type::DIR_READ> reading;
        Await<Copy, msg::file::Copy, msg::Type::FILE_COPY> copying;
        Await<Copy, npos::fs::msg::dir::Close, npos::fs::msg::Type::DIR_CLOSE> closing;
    };
}
