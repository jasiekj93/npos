#pragma once

/**
 * @file Cp.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <libnpos/mdlwr/cli/utils/Transfer.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Copy.hpp>
#include <libnpos/mdlwr/fs/msg/file/Copy.hpp>

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
                mdlwr::fs::msg::dir::Copy request(npos::ipc::NULL_OID, source, target);
                getApi().send(*this, request);
                return request.type;
            }

            mdlwr::fs::msg::file::Copy request(npos::ipc::NULL_OID, source, target);
            getApi().send(*this, request);
            return request.type;
        }
    };
}
