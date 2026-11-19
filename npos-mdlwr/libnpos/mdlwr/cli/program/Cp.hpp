#pragma once

/**
 * @file Cp.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Transfer.hpp>
#include <libnpos/mdlwr/fs/Api.hpp>

namespace npos::mdlwr::cli::program
{
    // cp [-r] <source>... <destination> - copy files, and directories with -r (or -R).
    class Cp : public utils::Transfer
    {
    public:
        explicit Cp(Api& api)
            : utils::Transfer(api, "cp", "cp [-r] <source>... <destination> - copy files and directories",
                "usage: cp [-r] <source>... <destination>\r\n", "r", "cannot copy")
        {
        }

    protected:
        bool directoriesAllowed() const override
        {
            return recursive;
        }

        npos::ipc::Message::Type send(bool directory, etl::string_view source, etl::string_view target) override
        {
            if(directory)
            {
                fs::api::dir::copy(*this, source, target);
                return fs::msg::Type::DIR_COPY;
            }

            fs::api::file::copy(*this, source, target);
            return fs::msg::Type::FILE_COPY;
        }
    };
}
