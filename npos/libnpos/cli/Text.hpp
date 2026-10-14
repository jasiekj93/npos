#pragma once

/**
 * @file Text.hpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <cstdint>

#include <etl/string.h>
#include <etl/string_stream.h>

#include <libnpos/fs/msg/Status.hpp>
#include <libnpos/cli/Log.hpp>

namespace npos::cli::text
{
    static constexpr size_t MAX_PATH = 64;

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

    // Applies `path` to the absolute path in `out`, handling ".", ".." and repeated slashes.
    inline bool apply(etl::istring& out, etl::string_view path)
    {
        while(not path.empty())
        {
            auto slash = path.find('/');
            auto length = (slash == etl::string_view::npos) ? path.size() : slash;
            auto segment = path.substr(0, length);
            path.remove_prefix(slash == etl::string_view::npos ? path.size() : slash + 1);

            if(segment.empty() or segment == ".")
                continue;

            if(segment == "..")
            {
                auto last = out.rfind('/');
                out.resize(last == 0 ? 1 : last);
                continue;
            }

            auto needed = segment.size() + (out.size() > 1 ? 1 : 0);
            if(out.size() + needed > MAX_PATH)
                return false;

            if(out.size() > 1)
                out.push_back('/');

            out.append(segment.data(), segment.size());
        }

        return true;
    }

    // Makes an absolute, normalized path from `path` relative to `directory`; false when longer than MAX_PATH.
    inline bool resolve(etl::string_view directory, etl::string_view path, etl::istring& out)
    {
        out.assign("/");

        if(path.empty() or path.front() != '/')
            if(not apply(out, directory))
                return false;

        return apply(out, path);
    }

    inline void appendNumber(etl::istring& out, uint32_t value, size_t width = 0)
    {
        char digits[10];
        size_t length = 0;

        do
        {
            digits[length++] = static_cast<char>('0' + value % 10);
            value /= 10;
        }
        while(value > 0);

        for(size_t i = length; i < width; i++)
            out.push_back(' ');

        while(length > 0)
            out.push_back(digits[--length]);
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
        case fs::msg::Status::END_OF_DIRECTORY: return "end of directory";
        default: return "unknown error";
        }
    }

    inline etl::string_view syslogLevel(log::Level level)
    {
        switch(level)
        {
        case log::Level::DEBUG: return "debug";
        case log::Level::INFO: return "info";
        case log::Level::WARNING: return "warning";
        case log::Level::ERROR: return "error";
        default: return "unknown";
        }
    }

    inline etl::string<10> printOid(ipc::Oid oid)
    {
        etl::string<10> out;
        etl::string_stream stream(out);

        stream << "{" << static_cast<int>(oid.portId) << ":" << static_cast<int>(oid.serviceId) << "}";
        return out;
    }
}
