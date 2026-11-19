#pragma once

/**
 * @file Mv.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Transfer.hpp>
#include <libnpos/mdlwr/fs/Api.hpp>

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
                fs::api::dir::move(*this, source, target);
                return fs::msg::Type::DIR_MOVE;
            }

            fs::api::file::move(*this, source, target);
            return fs::msg::Type::FILE_MOVE;
        }
    };
}
