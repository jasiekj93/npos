/**
 * @file Text.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/cli/Text.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::cli;

TEST_GROUP(Text)
{
    etl::string<text::MAX_PATH> path;

    bool resolve(const char* directory, const char* argument)
    {
        return text::resolve(directory, argument, path);
    }
};

TEST(Text, PathLimitIsSixtyFourCharacters)
{
    CHECK_EQUAL(64, text::MAX_PATH);
}

TEST(Text, TrimRemovesSurroundingSpaces)
{
    CHECK_TRUE(text::trim("  a b  ") == "a b");
    CHECK_TRUE(text::trim("   ").empty());
    CHECK_TRUE(text::trim("").empty());
    CHECK_TRUE(text::trim("x") == "x");
}

TEST(Text, TokenCutsFirstWord)
{
    etl::string_view rest = "  one  two three ";

    CHECK_TRUE(text::token(rest) == "one");
    CHECK_TRUE(rest == "two three");
    CHECK_TRUE(text::token(rest) == "two");
    CHECK_TRUE(text::token(rest) == "three");
    CHECK_TRUE(rest.empty());
    CHECK_TRUE(text::token(rest).empty());
}

TEST(Text, ResolveKeepsAbsolutePath)
{
    CHECK_TRUE(resolve("/dir", "/a/b"));
    STRCMP_EQUAL("/a/b", path.c_str());
}

TEST(Text, ResolveJoinsRelativePathWithDirectory)
{
    CHECK_TRUE(resolve("/dir", "a.txt"));
    STRCMP_EQUAL("/dir/a.txt", path.c_str());
}

TEST(Text, ResolveOfEmptyPathIsTheDirectory)
{
    CHECK_TRUE(resolve("/dir/sub", ""));
    STRCMP_EQUAL("/dir/sub", path.c_str());
}

TEST(Text, ResolveOfRootIsRoot)
{
    CHECK_TRUE(resolve("/", ""));
    STRCMP_EQUAL("/", path.c_str());
}

TEST(Text, ResolveHandlesDotAndRepeatedSlashes)
{
    CHECK_TRUE(resolve("/a", ".//b///./c/"));
    STRCMP_EQUAL("/a/b/c", path.c_str());
}

TEST(Text, ResolveHandlesParentDirectory)
{
    CHECK_TRUE(resolve("/a/b", ".."));
    STRCMP_EQUAL("/a", path.c_str());
    CHECK_TRUE(resolve("/a/b", "../c"));
    STRCMP_EQUAL("/a/c", path.c_str());
    CHECK_TRUE(resolve("/a/b", "../../.."));
    STRCMP_EQUAL("/", path.c_str());
}

TEST(Text, ResolveAcceptsPathOfExactlyMaximumLength)
{
    // "/" + 31 + "/" + 30 = 63, plus one more character gives 64
    etl::string<80> argument = "/";
    argument.append(31, 'a');
    argument.append("/");
    argument.append(31, 'b');
    CHECK_EQUAL(64, argument.size());

    CHECK_TRUE(text::resolve("/", argument.c_str(), path));
    CHECK_EQUAL(64, path.size());
}

TEST(Text, ResolveRejectsPathLongerThanMaximum)
{
    etl::string<80> argument = "/";
    argument.append(31, 'a');
    argument.append("/");
    argument.append(32, 'b');
    CHECK_EQUAL(65, argument.size());

    CHECK_FALSE(text::resolve("/", argument.c_str(), path));
}

TEST(Text, ResolveRejectsRelativePathThatOverflowsWithDirectory)
{
    etl::string<80> directory = "/";
    directory.append(40, 'd');

    CHECK_FALSE(text::resolve(directory.c_str(), "0123456789012345678901234", path));
}

TEST(Text, AppendNumberWritesDecimalDigits)
{
    etl::string<16> out;

    text::appendNumber(out, 0);
    text::appendNumber(out, 1205);
    text::appendNumber(out, 4294967295u);

    STRCMP_EQUAL("012054294967295", out.c_str());
}

TEST(Text, AppendNumberPadsToWidth)
{
    etl::string<16> out;

    text::appendNumber(out, 42, 5);

    STRCMP_EQUAL("   42", out.c_str());
}

TEST(Text, AppendNumberDoesNotTruncateWiderNumbers)
{
    etl::string<16> out;

    text::appendNumber(out, 12345, 2);

    STRCMP_EQUAL("12345", out.c_str());
}

TEST(Text, DescribeNamesFilesystemStatuses)
{
    using npos::fs::msg::Status;

    STRCMP_EQUAL("success", text::describe(Status::SUCCESS));
    STRCMP_EQUAL("no such file or directory", text::describe(Status::NO_DIR_ENTRY));
    STRCMP_EQUAL("is a directory", text::describe(Status::IS_A_DIRECTORY));
    STRCMP_EQUAL("not a directory", text::describe(Status::NOT_A_DIRECTORY));
    STRCMP_EQUAL("end of directory", text::describe(Status::END_OF_DIRECTORY));
    STRCMP_EQUAL("unknown error", text::describe(Status::UNKNOWN_ERROR));
}

TEST(Text, DescribeCoversEveryStatus)
{
    using npos::fs::msg::Status;

    for(int status = Status::SUCCESS; status <= Status::END_OF_DIRECTORY; status++)
        CHECK_TRUE(strlen(text::describe(static_cast<Status>(status))) > 0);
}

TEST(Text, DescribeFallsBackForUnknownValues)
{
    STRCMP_EQUAL("unknown error", text::describe(static_cast<npos::fs::msg::Status>(200)));
}
