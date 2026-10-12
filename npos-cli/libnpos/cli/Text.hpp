#pragma once

/**
 * @file Text.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <etl/string_view.h>

#include <libnpos/fs/msg/Status.hpp>

namespace npos::cli::text
{
    inline etl::string_view trim(etl::string_view text)
    {
        while(not text.empty() and text.front() == ' ')
            text.remove_prefix(1);

        while(not text.empty() and text.back() == ' ')
            text.remove_suffix(1);

        return text;
    }

    // Cuts the first space separated word off `rest` and returns it.
    inline etl::string_view token(etl::string_view& rest)
    {
        rest = trim(rest);

        auto end = rest.find(' ');
        if(end == etl::string_view::npos)
            end = rest.size();

        auto word = rest.substr(0, end);
        rest.remove_prefix(end);
        rest = trim(rest);
        return word;
    }

    inline const char* describe(fs::msg::Status status)
    {
        switch(status)
        {
        case fs::msg::Status::SUCCESS: return "success";
        case fs::msg::Status::IO_ERROR: return "I/O error";
        case fs::msg::Status::CORRUPTED: return "filesystem corrupted";
        case fs::msg::Status::NO_DIR_ENTRY: return "no such file or directory";
        case fs::msg::Status::ENTRY_EXISTS: return "file exists";
        case fs::msg::Status::NOT_A_DIRECTORY: return "not a directory";
        case fs::msg::Status::IS_A_DIRECTORY: return "is a directory";
        case fs::msg::Status::DIRECTORY_NOT_EMPTY: return "directory not empty";
        case fs::msg::Status::BAD_FILE_NUMBER: return "bad file number";
        case fs::msg::Status::FILE_TOO_LARGE: return "file too large";
        case fs::msg::Status::INVALID_PARAMETER: return "invalid parameter";
        case fs::msg::Status::NO_SPACE_LEFT: return "no space left";
        case fs::msg::Status::NO_MEMORY: return "out of memory";
        case fs::msg::Status::NO_ATTRIBUTE: return "no attribute";
        case fs::msg::Status::NAME_TOO_LONG: return "name too long";
        default: return "unknown error";
        }
    }
}
