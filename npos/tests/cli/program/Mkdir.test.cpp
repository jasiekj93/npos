/**
 * @file Mkdir.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Mkdir.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Mkdir)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Mkdir mkdir{api};
    mock::ProgramHarness filesystem{mkdir, api};

    void answer(msg::Status status)
    {
        auto message = filesystem.take();
        reinterpret_cast<msg::dir::Make&>(message).setStatus(status);
        mkdir.handle(message);
    }
};

TEST(Mkdir, HasNameAndDescription)
{
    CHECK_TRUE(mkdir.getName() == "mkdir");
    CHECK_TRUE(mkdir.getDescription().size() > 0);
}

TEST(Mkdir, WithoutArgumentsPrintsUsage)
{
    mkdir.start(host, "");

    CHECK_TRUE(host.outputIs("usage: mkdir <dir>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Mkdir, RequestsDirectoryRelativeToWorkingDirectory)
{
    host.cwd = "/a";

    mkdir.start(host, "b");

    auto request = filesystem.take();
    CHECK_EQUAL(msg::Type::DIR_MAKE, request.type);
    STRCMP_EQUAL("/a/b", reinterpret_cast<msg::dir::Make&>(request).getPath().data());
}

TEST(Mkdir, ExitsSilentlyOnSuccess)
{
    mkdir.start(host, "b");

    answer(msg::Status::SUCCESS);

    POINTERS_EQUAL(&mkdir, host.exited);
    CHECK_EQUAL(0, host.exitCode);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Mkdir, ReportsFailure)
{
    mkdir.start(host, "b");

    answer(msg::Status::ENTRY_EXISTS);

    CHECK_TRUE(host.outputIs("mkdir: /b: file exists\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Mkdir, TooLongPathIsRejected)
{
    etl::string<100> args(70, 'a');

    mkdir.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("mkdir: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Mkdir, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::DIR_MAKE;

    mkdir.handle(message);
    mkdir.start(host, "b");
    message.type = msg::Type::DIR_OPEN;
    mkdir.handle(message);

    CHECK_EQUAL(0, host.exitCount);
}
