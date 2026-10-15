/**
 * @file Cd.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Cd.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Cd)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Cd cd{api};
    mock::ProgramHarness filesystem{cd, api};

    const char* openedPath()
    {
        auto request = filesystem.take();
        static npos::ipc::Message kept;
        kept = request;
        return reinterpret_cast<msg::dir::Open&>(kept).getPath().data();
    }
};

TEST(Cd, HasNameAndDescription)
{
    CHECK_TRUE(cd.getName() == "cd");
    CHECK_TRUE(cd.getDescription().size() > 0);
}

TEST(Cd, OpensTargetDirectoryRelativeToWorkingDirectory)
{
    host.cwd = "/a";

    cd.start(host, "b");

    CHECK_EQUAL(msg::Type::DIR_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a/b", openedPath());
}

TEST(Cd, WithoutArgumentsGoesToRoot)
{
    host.cwd = "/a";

    cd.start(host, "");

    STRCMP_EQUAL("/", openedPath());
}

TEST(Cd, ChangesWorkingDirectoryAfterSuccessfulOpenAndClosesDirectory)
{
    cd.start(host, "sub");

    filesystem.answerDirOpen(msg::Status::SUCCESS, { 5, 1 });

    STRCMP_EQUAL("/sub", host.cwd.c_str());
    CHECK_EQUAL(msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerDirClose();

    POINTERS_EQUAL(&cd, host.exited);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Cd, ParentDirectoryIsResolved)
{
    host.cwd = "/a/b";

    cd.start(host, "..");
    filesystem.answerDirOpen(msg::Status::SUCCESS);

    STRCMP_EQUAL("/a", host.cwd.c_str());
}

TEST(Cd, FailureKeepsWorkingDirectoryAndReports)
{
    host.cwd = "/a";
    cd.start(host, "missing");

    filesystem.answerDirOpen(msg::Status::NO_DIR_ENTRY);

    STRCMP_EQUAL("/a", host.cwd.c_str());
    CHECK_TRUE(host.outputIs("cd: /a/missing: no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cd, FileIsNotADirectory)
{
    cd.start(host, "f");

    filesystem.answerDirOpen(msg::Status::NOT_A_DIRECTORY);

    CHECK_TRUE(host.outputIs("cd: /f: not a directory\r\n"));
}

TEST(Cd, TooLongPathIsRejected)
{
    etl::string<100> args(70, 'a');

    cd.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("cd: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cd, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::DIR_CLOSE;

    cd.handleResponse(message);
    message.type = msg::Type::DIR_OPEN;
    cd.handleResponse(message);

    CHECK_EQUAL(0, host.exitCount);
}
