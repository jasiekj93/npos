#pragma once

/**
 * @file Mv.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Transfer.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Move.hpp>
#include <libnpos/mdlwr/fs/msg/file/Move.hpp>

namespace npos::mdlwr::cli::program
{
    // mv <source>... <destination> - move files and directories. An existing destination file is replaced.
    class Mv : public utils::Transfer
    {
    public:
        explicit Mv(Api& api)
            : utils::Transfer(api, "mv", "mv <source>... <destination> - move files and directories",
                "usage: mv <source>... <destination>\r\n", "", "cannot move")
        {
        }

    protected:
        bool directoriesAllowed() const override
        {
            return true;
        }

        npos::ipc::Message::Type send(bool directory, etl::string_view source, etl::string_view target) override
        {
            if(directory)
            {
                mdlwr::fs::msg::dir::Move request(npos::ipc::NULL_OID, source, target);
                getApi().send(*this, request);
                return request.type;
            }

            mdlwr::fs::msg::file::Move request(npos::ipc::NULL_OID, source, target);
            getApi().send(*this, request);
            return request.type;
        }
    };
}
