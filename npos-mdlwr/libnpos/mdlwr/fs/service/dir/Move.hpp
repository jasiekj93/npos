#pragma once

/**
 * @file Move.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/fs/msg/dir/Move.hpp>
#include <libnpos/mdlwr/fs/service/Operation.hpp>
#include <libnpos/mdlwr/fs/utils/Path.hpp>
#include <libnpos/mdlwr/fs/utils/Status.hpp>

namespace npos::mdlwr::fs::service::dir
{
    // Moves a directory with everything inside by renaming it. The destination must not exist.
    class Move : public Operation<msg::dir::Move>
    {
    public:
        explicit Move(Api& api)
            : Operation(api, states, msg::Type::DIR_MOVE)
            , idle(*this)
            , checkingSource(CHECKING_SOURCE, *this, &Move::onSourceChecked)
            , checkingDestination(CHECKING_DESTINATION, *this, &Move::onDestinationChecked)
            , renaming(RENAMING, *this, &Move::onRenamed)
        {
            states[IDLE] = &idle;
            states[CHECKING_SOURCE] = &checkingSource;
            states[CHECKING_DESTINATION] = &checkingDestination;
            states[RENAMING] = &renaming;
        }

    protected:
        ID begin(msg::dir::Move& request) override
        {
            if(not utils::assign(source, request.getOldPath()) or not utils::assign(destination, request.getNewPath()))
                return refuse(request, msg::Status::INVALID_PATH);

            if(utils::isInside(utils::view(source), utils::view(destination)))
                return refuse(request, msg::Status::INVALID_PATH);

            current.store(request);

            npos::fs::api::stat(*this, utils::view(source), info);
            return CHECKING_SOURCE;
        }

    private:
        enum : ID
        {
            CHECKING_SOURCE = IDLE + 1,
            CHECKING_DESTINATION,
            RENAMING,
            STATES
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
        Await<Move, npos::fs::msg::Stat, npos::fs::msg::Type::STAT> checkingDestination;
        Await<Move, npos::fs::msg::Rename, npos::fs::msg::Type::RENAME> renaming;
    };
}
