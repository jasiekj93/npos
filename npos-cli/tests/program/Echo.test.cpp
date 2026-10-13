/**
 * @file Echo.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Echo.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Echo)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Echo echo{api};
    mock::ProgramHarness filesystem{echo, api};
};

TEST(Echo, HasNameAndDescription)
{
    CHECK_TRUE(echo.getName() == "echo");
    CHECK_TRUE(echo.getDescription().size() > 0);
}

TEST(Echo, PrintsTrimmedText)
{
    echo.start(host, "  hello world  ");

    CHECK_TRUE(host.outputIs("hello world\r\n"));
    CHECK_EQUAL(0, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Echo, TruncatesFileWithSingleRedirect)
{
    host.cwd = "/d";

    echo.start(host, "hi > f.txt");

    auto request = filesystem.take();
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    STRCMP_EQUAL("/d/f.txt", open.getPath().data());
    CHECK_EQUAL(Flags::WRITE_ONLY | Flags::CREATE | Flags::TRUNCATE, open.getFlags());
}

TEST(Echo, AppendsWithDoubleRedirect)
{
    echo.start(host, "hi >> f.txt");

    auto request = filesystem.take();
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    CHECK_EQUAL(Flags::WRITE_ONLY | Flags::CREATE | Flags::APPEND, open.getFlags());
}

TEST(Echo, WritesTextWithNewlineThenClosesFile)
{
    echo.start(host, "hi > f.txt");
    filesystem.answerOpen(msg::Status::SUCCESS, { 5, 3 });

    CHECK_EQUAL(msg::Type::FILE_WRITE, filesystem.nextType());
    auto& request = api.sent.front();
    CHECK_EQUAL(3, request.recipient.serviceId);
    CHECK_EQUAL(3, request.input.size);
    MEMCMP_EQUAL("hi\n", request.input.data, 3);

    filesystem.answerWrite();

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerClose();

    POINTERS_EQUAL(&echo, host.exited);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Echo, RedirectWithoutFileNamePrintsUsage)
{
    echo.start(host, "hi >");

    CHECK_TRUE(host.outputIs("usage: echo <text> [> | >> <file>]\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Echo, TextThatDoesNotFitInBufferPrintsUsage)
{
    etl::string<160> args(96, 'x');
    args.append(" > f");

    echo.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Echo, LongestTextStillFits)
{
    etl::string<160> args(95, 'x');
    args.append(" > f");

    echo.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(filesystem.hasRequest());
}

TEST(Echo, TooLongPathIsRejected)
{
    etl::string<160> args("hi > ");
    args.append(70, 'a');

    echo.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("echo: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Echo, OpenFailureIsReported)
{
    echo.start(host, "hi > f");

    filesystem.answerOpen(msg::Status::IS_A_DIRECTORY);

    CHECK_TRUE(host.outputIs("echo: /f: is a directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Echo, WriteFailureIsReportedAndFileStillClosed)
{
    echo.start(host, "hi > f");
    filesystem.answerOpen(msg::Status::SUCCESS);

    filesystem.answerWrite(msg::Status::NO_SPACE_LEFT);

    CHECK_TRUE(host.outputIs("echo: /f: no space left\r\n"));
    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());

    filesystem.answerClose();

    CHECK_EQUAL(1, host.exitCode);
}

TEST(Echo, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_WRITE;

    echo.handle(message);
    message.type = npos::ipc::DEVICE_MESSAGE_TYPE;
    echo.handle(message);

    CHECK_EQUAL(0, host.exitCount);
}
