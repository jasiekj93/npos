#pragma once

/**
 * @file Path.hpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include <etl/string.h>
#include <etl/string_view.h>

namespace npos::mdlwr::fs::utils
{
    static constexpr size_t MAX_PATH = 64;

    using Path = etl::string<MAX_PATH>;

    // The filesystem needs null terminated paths, which the string_view of a message does not guarantee.
    inline etl::string_view view(const Path& path)
    {
        return etl::string_view(path.c_str(), path.size());
    }

    // False when the text is empty or longer than MAX_PATH.
    inline bool assign(Path& path, etl::string_view text)
    {
        if(text.empty() or text.size() > path.capacity())
            return false;

        path.assign(text.data(), text.size());
        return true;
    }

    // Adds one more segment to the path; false when it would not fit.
    inline bool append(Path& path, etl::string_view name)
    {
        auto separator = (path.back() == '/') ? 0 : 1;

        if(path.size() + separator + name.size() > path.capacity())
            return false;

        if(separator)
            path.push_back('/');

        path.append(name.data(), name.size());
        return true;
    }

    // Cuts the last segment off the path.
    inline void removeLast(Path& path)
    {
        auto last = path.rfind('/');

        if(last != Path::npos)
            path.resize(last == 0 ? 1 : last);
    }

    // True when `path` lies somewhere under `directory`.
    inline bool isInside(etl::string_view directory, etl::string_view path)
    {
        if(path.size() <= directory.size() or path.substr(0, directory.size()) != directory)
            return false;

        return directory.back() == '/' or path[directory.size()] == '/';
    }
}
