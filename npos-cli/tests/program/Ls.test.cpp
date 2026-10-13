/**
 * @file Ls.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Ls.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Ls)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Ls ls{api};
    mock::ProgramHarness filesystem{ls, api};

    void startOpened(const char* args = "")
    {
        ls.start(host, args);
        filesystem.answerDirOpen(msg::Status::SUCCESS, { 5, 4 });
    }
};

TEST(Ls, HasNameAndDescription)
{
    CHECK_TRUE(ls.getName() == "ls");
    CHECK_TRUE(ls.getDescription().size() > 0);
}

TEST(Ls, ListsWorkingDirectoryByDefault)
{
    host.cwd = "/data";

    ls.start(host, "");

    auto request = filesystem.take();
    CHECK_EQUAL(msg::Type::DIR_OPEN, request.type);
    STRCMP_EQUAL("/data", reinterpret_cast<msg::dir::Open&>(request).getPath().data());
}

TEST(Ls, ListsGivenDirectory)
{
    ls.start(host, "x/y");

    auto request = filesystem.take();
    STRCMP_EQUAL("/x/y", reinterpret_cast<msg::dir::Open&>(request).getPath().data());
}

TEST(Ls, ReadsEntriesFromOpenedDirectory)
{
    startOpened();

    CHECK_EQUAL(msg::Type::DIR_READ, filesystem.nextType());
    auto request = filesystem.take();
    CHECK_EQUAL(4, request.recipient.serviceId);
}

TEST(Ls, PrintsFilesWithSizeAndDirectoriesWithSlash)
{
    startOpened();

    filesystem.answerDirRead(msg::Status::SUCCESS, msg::dir::Entry::FILE, 5, "a.txt");
    filesystem.answerDirRead(msg::Status::SUCCESS, msg::dir::Entry::DIRECTORY, 0, "sub");

    CHECK_TRUE(host.outputIs("         5 a.txt\r\n     <dir> sub/\r\n"));
    CHECK_EQUAL(msg::Type::DIR_READ, filesystem.nextType());
}

TEST(Ls, ClosesDirectoryAtTheEndAndExits)
{
    startOpened();

    filesystem.answerDirRead(msg::Status::END_OF_DIRECTORY);

    CHECK_EQUAL(msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerDirClose();

    POINTERS_EQUAL(&ls, host.exited);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Ls, OpenFailureIsReported)
{
    ls.start(host, "nope");

    filesystem.answerDirOpen(msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("ls: /nope: no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Ls, ReadFailureIsReportedAndDirectoryClosed)
{
    startOpened();

    filesystem.answerDirRead(msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("ls: /: I/O error\r\n"));
    CHECK_EQUAL(msg::Type::DIR_CLOSE, filesystem.nextType());

    filesystem.answerDirClose();

    CHECK_EQUAL(1, host.exitCode);
}

TEST(Ls, WaitsForOutputToDrainAndContinuesOnResume)
{
    startOpened();
    host.capacity = 4;

    filesystem.answerDirRead(msg::Status::SUCCESS, msg::dir::Entry::FILE, 7, "f");

    CHECK_EQUAL(4, host.output.size());
    CHECK_FALSE(filesystem.hasRequest());

    host.capacity = 100;
    ls.resume();

    CHECK_TRUE(host.outputIs("         7 f\r\n"));
    CHECK_EQUAL(msg::Type::DIR_READ, filesystem.nextType());
}

TEST(Ls, ResumeOutsideOfPrintingDoesNothing)
{
    startOpened();

    ls.resume();

    CHECK_EQUAL(1, api.sent.size());
}

TEST(Ls, TooLongPathIsRejected)
{
    etl::string<100> args(70, 'a');

    ls.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("ls: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Ls, IgnoresResponsesOutOfOrder)
{
    npos::ipc::Message message;
    message.type = msg::Type::DIR_READ;
    message.output.status = msg::Status::SUCCESS;

    ls.handle(message);
    message.type = msg::Type::DIR_OPEN;
    ls.handle(message);
    message.type = msg::Type::DIR_CLOSE;
    ls.handle(message);
    message.type = npos::ipc::DEVICE_MESSAGE_TYPE;
    ls.handle(message);

    CHECK_TRUE(host.outputIs(""));
    CHECK_EQUAL(0, host.exitCount);
}
