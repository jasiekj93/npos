#pragma once

/**
 * @file Move.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/fs/msg/file/Move.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::file
{
    // Removes the destination when it exists and renames the source.
    class Move : public Operation<msg::file::Move>
    {
    public:
        explicit Move(Api& api)
            : Operation(api, states, msg::Type::FILE_MOVE)
            , idle(*this)
            , checkingSource(CHECKING_SOURCE, *this, &Move::onSourceChecked)
            , removingDestination(REMOVING_DESTINATION, *this, &Move::onDestinationRemoved)
            , renaming(RENAMING, *this, &Move::onRenamed)
        {
            states[IDLE] = &idle;
            states[CHECKING_SOURCE] = &checkingSource;
            states[REMOVING_DESTINATION] = &removingDestination;
            states[RENAMING] = &renaming;
        }

    protected:
        ID begin(msg::file::Move& request) override
        {
            if(not utils::assign(source, request.getOldPath()) or not utils::assign(destination, request.getNewPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);

            // The source is checked first, otherwise a missing source would still cost the destination.
            npos::fs::api::stat(*this, utils::view(source), info);
            return CHECKING_SOURCE;
        }

    private:
        enum : ID
        {
            CHECKING_SOURCE = IDLE + 1,
            REMOVING_DESTINATION,
            RENAMING,
            STATES
        };

        ID onSourceChecked(npos::fs::msg::Stat& response)
        {
            if(response.getStatus() != npos::fs::msg::Status::SUCCESS)
                return finish(utils::fromFsStatus(response.getStatus(), msg::Status::FILE_NOT_FOUND));

            if(info.type == npos::fs::Info::DIRECTORY)
                return finish(msg::Status::NOT_A_FILE);

            // Removing the destination would remove the source.
            if(source == destination)
                return finish(msg::Status::SUCCESS);

            npos::fs::api::file::remove(*this, utils::view(destination));
            return REMOVING_DESTINATION;
        }

        ID onDestinationRemoved(npos::fs::msg::file::Remove& response)
        {
            auto status = response.getStatus();

            if(status != npos::fs::msg::Status::SUCCESS and status != npos::fs::msg::Status::NO_DIR_ENTRY)
                return finish(utils::fromFsStatus(status, msg::Status::DEVICE_ERROR));

            npos::fs::api::rename(*this, utils::view(source), utils::view(destination));
            return RENAMING;
        }

        ID onRenamed(npos::fs::msg::Rename& response)
        {
            return finish(utils::fromFsStatus(response.getStatus(), msg::Status::DIR_NOT_FOUND));
        }

        utils::Path source;
        utils::Path destination;
        npos::fs::Info info;

        State* states[STATES] = {};
        Idle idle;
        Await<Move, npos::fs::msg::Stat, npos::fs::msg::Type::STAT> checkingSource;
        Await<Move, npos::fs::msg::file::Remove, npos::fs::msg::Type::FILE_REMOVE> removingDestination;
        Await<Move, npos::fs::msg::Rename, npos::fs::msg::Type::RENAME> renaming;
    };
}
