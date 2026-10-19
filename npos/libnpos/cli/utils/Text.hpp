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

namespace npos::cli::utils::text
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

    inline void appendNumber(etl::istring& out, uint32_t value, size_t width = 0, char fill = ' ')
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
            out.push_back(fill);

        while(length > 0)
            out.push_back(digits[--length]);
    }

    inline void appendHex(etl::istring& out, uint8_t value)
    {
        static constexpr char DIGITS[] = "0123456789abcdef";

        out.push_back(DIGITS[value >> 4]);
        out.push_back(DIGITS[value & 0x0f]);
    }

    // Whole text must be decimal digits and fit in 32 bits.
    inline bool parseNumber(etl::string_view text, uint32_t& value)
    {
        if(text.empty())
            return false;

        uint64_t result = 0;

        for(auto c : text)
        {
            if(c < '0' or c > '9')
                return false;

            result = result * 10 + static_cast<uint32_t>(c - '0');

            if(result > UINT32_MAX)
                return false;
        }

        value = static_cast<uint32_t>(result);
        return true;
    }

    // Calendar fields of a UTC timestamp (seconds since 1970-01-01).
    struct DateTime
    {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    inline bool isLeapYear(uint32_t year)
    {
        return (year % 4 == 0 and year % 100 != 0) or year % 400 == 0;
    }

    inline uint32_t daysInMonth(uint32_t year, uint32_t month)
    {
        static constexpr uint8_t DAYS[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        return (month == 2 and isLeapYear(year)) ? 29 : DAYS[month - 1];
    }

    // Timestamps start in 1970.
    inline bool isValid(const DateTime& dateTime)
    {
        return dateTime.year >= 1970 and
            dateTime.month >= 1 and dateTime.month <= 12 and
            dateTime.day >= 1 and dateTime.day <= daysInMonth(dateTime.year, dateTime.month) and
            dateTime.hour < 24 and dateTime.minute < 60 and dateTime.second < 60;
    }

    // Civil from days, after Howard Hinnant.
    inline DateTime toDateTime(uint64_t timestamp)
    {
        uint64_t days = timestamp / 86400 + 719468;
        uint32_t secondsOfDay = static_cast<uint32_t>(timestamp % 86400);

        uint64_t era = days / 146097;
        uint32_t dayOfEra = static_cast<uint32_t>(days - era * 146097);
        uint32_t yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
        uint32_t dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
        uint32_t monthPrime = (5 * dayOfYear + 2) / 153;

        uint32_t day = dayOfYear - (153 * monthPrime + 2) / 5 + 1;
        uint32_t month = monthPrime < 10 ? monthPrime + 3 : monthPrime - 9;
        uint32_t year = static_cast<uint32_t>(yearOfEra + era * 400) + (month <= 2 ? 1 : 0);

        DateTime dateTime;
        dateTime.year = static_cast<uint16_t>(year);
        dateTime.month = static_cast<uint8_t>(month);
        dateTime.day = static_cast<uint8_t>(day);
        dateTime.hour = static_cast<uint8_t>(secondsOfDay / 3600);
        dateTime.minute = static_cast<uint8_t>((secondsOfDay % 3600) / 60);
        dateTime.second = static_cast<uint8_t>(secondsOfDay % 60);
        return dateTime;
    }

    // False when the fields are not a valid date.
    inline bool toTimestamp(const DateTime& dateTime, uint64_t& timestamp)
    {
        if(not isValid(dateTime))
            return false;

        uint32_t year = dateTime.year - (dateTime.month <= 2 ? 1 : 0);
        uint32_t era = year / 400;
        uint32_t yearOfEra = year - era * 400;
        uint32_t monthPrime = dateTime.month > 2 ? dateTime.month - 3 : dateTime.month + 9;
        uint32_t dayOfYear = (153 * monthPrime + 2) / 5 + dateTime.day - 1;
        uint32_t dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
        uint64_t days = static_cast<uint64_t>(era) * 146097 + dayOfEra - 719468;

        timestamp = days * 86400 + dateTime.hour * 3600u + dateTime.minute * 60u + dateTime.second;
        return true;
    }

    // YYYY-MM-DD HH:MM:SS
    inline void formatDateTime(etl::istring& out, const DateTime& dateTime)
    {
        appendNumber(out, dateTime.year, 4, '0');
        out.push_back('-');
        appendNumber(out, dateTime.month, 2, '0');
        out.push_back('-');
        appendNumber(out, dateTime.day, 2, '0');
        out.push_back(' ');
        appendNumber(out, dateTime.hour, 2, '0');
        out.push_back(':');
        appendNumber(out, dateTime.minute, 2, '0');
        out.push_back(':');
        appendNumber(out, dateTime.second, 2, '0');
    }

    // YYYY-MM-DD with an optional HH:MM[:SS] after a space or 'T'.
    inline bool parseDateTime(etl::string_view text, DateTime& result)
    {
        text = trim(text);
        uint32_t fields[6] = {};

        auto digits = [&text](size_t count, uint32_t& value) -> bool
        {
            if(text.size() < count)
                return false;

            value = 0;

            for(size_t i = 0; i < count; i++)
            {
                if(text[i] < '0' or text[i] > '9')
                    return false;

                value = value * 10 + static_cast<uint32_t>(text[i] - '0');
            }

            text.remove_prefix(count);
            return true;
        };

        auto separator = [&text](char c) -> bool
        {
            if(text.empty() or text.front() != c)
                return false;

            text.remove_prefix(1);
            return true;
        };

        if(not digits(4, fields[0]) or not separator('-') or not digits(2, fields[1]) or
            not separator('-') or not digits(2, fields[2]))
            return false;

        if(not text.empty())
        {
            if(not separator(' ') and not separator('T'))
                return false;

            text = trim(text);

            if(not digits(2, fields[3]) or not separator(':') or not digits(2, fields[4]))
                return false;

            if(not text.empty() and (not separator(':') or not digits(2, fields[5])))
                return false;
        }

        if(not text.empty())
            return false;

        DateTime parsed;
        parsed.year = static_cast<uint16_t>(fields[0]);
        parsed.month = static_cast<uint8_t>(fields[1]);
        parsed.day = static_cast<uint8_t>(fields[2]);
        parsed.hour = static_cast<uint8_t>(fields[3]);
        parsed.minute = static_cast<uint8_t>(fields[4]);
        parsed.second = static_cast<uint8_t>(fields[5]);

        if(not isValid(parsed))
            return false;

        result = parsed;
        return true;
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
