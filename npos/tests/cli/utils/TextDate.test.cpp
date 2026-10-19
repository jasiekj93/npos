/**
 * @file TextDate.test.cpp
 * @author Adrian Szczepanski
 * @date 19-10-2026
 */

#include <libnpos/cli/utils/Text.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::cli::utils;

TEST_GROUP(TextDate)
{
    etl::string<32> out;

    text::DateTime make(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
    {
        return text::DateTime{ year, month, day, hour, minute, second };
    }

    void checkDateTime(const text::DateTime& dateTime, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
    {
        CHECK_EQUAL(year, dateTime.year);
        CHECK_EQUAL(month, dateTime.month);
        CHECK_EQUAL(day, dateTime.day);
        CHECK_EQUAL(hour, dateTime.hour);
        CHECK_EQUAL(minute, dateTime.minute);
        CHECK_EQUAL(second, dateTime.second);
    }

    bool parse(const char* input, text::DateTime& dateTime) { return text::parseDateTime(input, dateTime); }
};

TEST(TextDate, AppendNumberPadsWithSpacesByDefault)
{
    text::appendNumber(out, 5, 3);

    STRCMP_EQUAL("  5", out.c_str());
}

TEST(TextDate, AppendNumberPadsWithTheGivenFill)
{
    text::appendNumber(out, 5, 2, '0');
    text::appendNumber(out, 1234, 2, '0');
    text::appendNumber(out, 0, 3, '0');

    STRCMP_EQUAL("05" "1234" "000", out.c_str());
}

TEST(TextDate, AppendHexWritesTwoLowercaseDigits)
{
    text::appendHex(out, 0x00);
    text::appendHex(out, 0x0a);
    text::appendHex(out, 0xff);
    text::appendHex(out, 0x5c);

    STRCMP_EQUAL("000aff5c", out.c_str());
}

TEST(TextDate, ParseNumberAcceptsDecimalDigits)
{
    uint32_t value = 0;

    CHECK_TRUE(text::parseNumber("0", value));
    CHECK_EQUAL(0, value);
    CHECK_TRUE(text::parseNumber("1024", value));
    CHECK_EQUAL(1024, value);
    CHECK_TRUE(text::parseNumber("4294967295", value));
    CHECK_EQUAL(4294967295u, value);
}

TEST(TextDate, ParseNumberRejectsEverythingElse)
{
    uint32_t value = 7;

    CHECK_FALSE(text::parseNumber("", value));
    CHECK_FALSE(text::parseNumber("12a", value));
    CHECK_FALSE(text::parseNumber("-1", value));
    CHECK_FALSE(text::parseNumber(" 1", value));
    CHECK_FALSE(text::parseNumber("4294967296", value));
    CHECK_FALSE(text::parseNumber("99999999999999999999", value));
    CHECK_EQUAL(7, value);
}

TEST(TextDate, TimestampZeroIsTheEpoch)
{
    checkDateTime(text::toDateTime(0), 1970, 1, 1, 0, 0, 0);
}

TEST(TextDate, ConvertsTimestampToDateAndTime)
{
    checkDateTime(text::toDateTime(1790686224), 2026, 9, 29, 12, 50, 24);
}

TEST(TextDate, ConvertsRangeBoundariesOfTheClock)
{
    checkDateTime(text::toDateTime(946684800), 2000, 1, 1, 0, 0, 0);
    checkDateTime(text::toDateTime(4102444799), 2099, 12, 31, 23, 59, 59);
}

TEST(TextDate, ConvertsLeapDays)
{
    checkDateTime(text::toDateTime(1709164800), 2024, 2, 29, 0, 0, 0);
    checkDateTime(text::toDateTime(1709251200), 2024, 3, 1, 0, 0, 0);
    checkDateTime(text::toDateTime(951782400), 2000, 2, 29, 0, 0, 0);
}

TEST(TextDate, ConvertsDateToTimestamp)
{
    uint64_t timestamp = 0;

    CHECK_TRUE(text::toTimestamp(make(2026, 9, 29, 12, 50, 24), timestamp));
    CHECK_EQUAL(1790686224, timestamp);
    CHECK_TRUE(text::toTimestamp(make(1970, 1, 1), timestamp));
    CHECK_EQUAL(0, timestamp);
    CHECK_TRUE(text::toTimestamp(make(2024, 2, 29), timestamp));
    CHECK_EQUAL(1709164800, timestamp);
}

TEST(TextDate, TimestampAndDateConvertBackAndForth)
{
    for(uint64_t timestamp = 946684800; timestamp < 4102444800; timestamp += 86400u * 37 + 12345)
    {
        uint64_t back = 0;

        CHECK_TRUE(text::toTimestamp(text::toDateTime(timestamp), back));
        CHECK_EQUAL(timestamp, back);
    }
}

TEST(TextDate, RejectsInvalidDates)
{
    uint64_t timestamp = 0;

    CHECK_FALSE(text::toTimestamp(make(2026, 2, 30), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2023, 2, 29), timestamp));
    CHECK_FALSE(text::toTimestamp(make(1900, 2, 29), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 13, 1), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 0, 1), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 4, 31), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 1, 1, 24), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 1, 1, 0, 60), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2026, 1, 1, 0, 0, 60), timestamp));
    CHECK_FALSE(text::toTimestamp(make(1969, 12, 31), timestamp));
}

TEST(TextDate, AcceptsLeapDaysOfLeapYearsOnly)
{
    uint64_t timestamp = 0;

    CHECK_TRUE(text::toTimestamp(make(2024, 2, 29), timestamp));
    CHECK_TRUE(text::toTimestamp(make(2000, 2, 29), timestamp));
    CHECK_FALSE(text::toTimestamp(make(2100, 2, 29), timestamp));
}

TEST(TextDate, FormatsWithLeadingZeros)
{
    text::formatDateTime(out, make(2026, 1, 2, 3, 4, 5));

    STRCMP_EQUAL("2026-01-02 03:04:05", out.c_str());
}

TEST(TextDate, FormatsTwoDigitFields)
{
    text::formatDateTime(out, make(2099, 12, 31, 23, 59, 59));

    STRCMP_EQUAL("2099-12-31 23:59:59", out.c_str());
}

TEST(TextDate, ParsesDateWithTimeAndSeconds)
{
    text::DateTime dateTime;

    CHECK_TRUE(parse("2026-09-29 12:50:24", dateTime));
    checkDateTime(dateTime, 2026, 9, 29, 12, 50, 24);
}

TEST(TextDate, ParsesTimeWithoutSeconds)
{
    text::DateTime dateTime;

    CHECK_TRUE(parse("2026-09-29 12:50", dateTime));
    checkDateTime(dateTime, 2026, 9, 29, 12, 50, 0);
}

TEST(TextDate, ParsesDateOnlyAsMidnight)
{
    text::DateTime dateTime;

    CHECK_TRUE(parse("2026-09-29", dateTime));
    checkDateTime(dateTime, 2026, 9, 29, 0, 0, 0);
}

TEST(TextDate, ParsesTSeparatorAndSurroundingSpaces)
{
    text::DateTime dateTime;

    CHECK_TRUE(parse("2026-09-29T12:50:24", dateTime));
    checkDateTime(dateTime, 2026, 9, 29, 12, 50, 24);
    CHECK_TRUE(parse("  2026-09-29   12:50  ", dateTime));
    checkDateTime(dateTime, 2026, 9, 29, 12, 50, 0);
}

TEST(TextDate, ParseRejectsMalformedText)
{
    text::DateTime dateTime = make(1, 1, 1);

    CHECK_FALSE(parse("", dateTime));
    CHECK_FALSE(parse("abc", dateTime));
    CHECK_FALSE(parse("2026-9-29", dateTime));
    CHECK_FALSE(parse("2026/09/29", dateTime));
    CHECK_FALSE(parse("2026-09-29 12", dateTime));
    CHECK_FALSE(parse("2026-09-29 12:5", dateTime));
    CHECK_FALSE(parse("2026-09-29 12:50:2", dateTime));
    CHECK_FALSE(parse("2026-09-29 12:50:24:11", dateTime));
    CHECK_FALSE(parse("2026-09-29x", dateTime));
    CHECK_EQUAL(1, dateTime.year);
}

TEST(TextDate, ParseRejectsInvalidValues)
{
    text::DateTime dateTime;

    CHECK_FALSE(parse("2026-13-01", dateTime));
    CHECK_FALSE(parse("2026-02-30", dateTime));
    CHECK_FALSE(parse("2026-09-29 25:00", dateTime));
    CHECK_FALSE(parse("2026-09-29 12:60", dateTime));
    CHECK_FALSE(parse("1969-12-31", dateTime));
}
