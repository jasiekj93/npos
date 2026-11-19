#pragma once

/**
 * @file Remove.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/fs/msg/dir/Remove.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::dir
{
    // Removes a directory together with everything inside. The filesystem removes only empty directories,
    // so the content goes first: the directory is opened, its first entry read and the directory closed again
    // before the entry is removed (or entered, when it is a directory). Only one directory is opened at a time
    // and the only memory of the traversal is the path.
    class Remove : public Operation<msg::dir::Remove>
    {
    public:
        explicit Remove(Api& api)
            : Operation(api, states, msg::Type::DIR_REMOVE)
            , idle(*this)
            , opening(OPENING, *this, &Remove::onOpened)
            , reading(READING, *this, &Remove::onRead)
            , closing(CLOSING, *this, &Remove::onClosed)
            , removingFile(REMOVING_FILE, *this, &Remove::onFileRemoved)
            , removingDirectory(REMOVING_DIRECTORY, *this, &Remove::onDirectoryRemoved)
        {
            states[IDLE] = &idle;
            states[OPENING] = &opening;
            states[READING] = &reading;
            states[CLOSING] = &closing;
            states[REMOVING_FILE] = &removingFile;
            states[REMOVING_DIRECTORY] = &removingDirectory;
        }

    protected:
        ID begin(msg::dir::Remove& request) override
        {
            // The root directory cannot be removed, emptying it would be a surprise.
            if(not utils::assign(path, request.getOldPath()) or path == "/")
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);
            depth = 0;
            directory = ipc::NULL_OID;
            result = msg::Status::SUCCESS;

            return open();
        }

    private:
        enum : ID
        {
            OPENING = IDLE + 1,
            READING,
            CLOSING,
            REMOVING_FILE,
            REMOVING_DIRECTORY,
            STATES
        };

        enum class After
        {
            REMOVE_ENTRY,
            REMOVE_DIRECTORY,
            FAIL
        };

        ID onOpened(npos::fs::msg::dir::Open& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));

            directory = response.getOpened();
            npos::fs::api::dir::read(*this, directory, entry);
            return READING;
        }

        ID onRead(npos::fs::msg::dir::Read& response)
        {
            switch(response.getStatus())
            {
            case npos::fs::msg::Status::SUCCESS:
                return close(After::REMOVE_ENTRY);
            case npos::fs::msg::Status::END_OF_DIRECTORY:
                return close(After::REMOVE_DIRECTORY);
            default:
                result = utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR);
                return close(After::FAIL);
            }
        }

        ID onClosed(npos::fs::msg::dir::Close& response)
        {
            if(after == After::FAIL)
                return finish(result);

            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            if(after == After::REMOVE_DIRECTORY)
            {
                npos::fs::api::dir::remove(*this, utils::view(path));
                return REMOVING_DIRECTORY;
            }

            if(not utils::append(path, entry.getName()))
                return finish(msg::Status::INVALID_PATH);

            if(entry.type == npos::fs::Info::DIRECTORY)
            {
                depth++;
                return open();
            }

            npos::fs::api::file::remove(*this, utils::view(path));
            return REMOVING_FILE;
        }

        ID onFileRemoved(npos::fs::msg::file::Remove& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            utils::removeLast(path);
            return open();
        }

        ID onDirectoryRemoved(npos::fs::msg::dir::Remove& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DEVICE_ERROR));

            if(depth == 0)
                return finish(msg::Status::SUCCESS);

            depth--;
            utils::removeLast(path);
            return open();
        }

        ID open()
        {
            npos::fs::api::dir::open(*this, utils::view(path));
            return OPENING;
        }

        ID close(After next)
        {
            after = next;
            npos::fs::api::dir::close(*this, directory);
            directory = ipc::NULL_OID;
            return CLOSING;
        }

        utils::Path path;
        npos::fs::Info entry;
        ipc::Oid directory = ipc::NULL_OID;
        size_t depth = 0;
        After after = After::FAIL;
        msg::Status result = msg::Status::SUCCESS;

        State* states[STATES] = {};
        Idle idle;
        Await<Remove, npos::fs::msg::dir::Open, npos::fs::msg::Type::DIR_OPEN> opening;
        Await<Remove, npos::fs::msg::dir::Read, npos::fs::msg::Type::DIR_READ> reading;
        Await<Remove, npos::fs::msg::dir::Close, npos::fs::msg::Type::DIR_CLOSE> closing;
        Await<Remove, npos::fs::msg::file::Remove, npos::fs::msg::Type::FILE_REMOVE> removingFile;
        Await<Remove, npos::fs::msg::dir::Remove, npos::fs::msg::Type::DIR_REMOVE> removingDirectory;
    };
}
