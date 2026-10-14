/**
 * @file Touch.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Touch.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Touch)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Touch touch{api};
    mock::ProgramHarness filesystem{touch, api};
};

TEST(Touch, HasNameAndDescription)
{
    CHECK_TRUE(touch.getName() == "touch");
    CHECK_TRUE(touch.getDescription().size() > 0);
}

TEST(Touch, WithoutArgumentsPrintsUsage)
{
    touch.start(host, "");

    CHECK_TRUE(host.outputIs("usage: touch <file>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Touch, CreatesFileRelativeToWorkingDirectory)
{
    host.cwd = "/d";

    touch.start(host, "new.txt");

    auto request = filesystem.take();
    CHECK_EQUAL(msg::Type::FILE_OPEN, request.type);
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    STRCMP_EQUAL("/d/new.txt", open.getPath().data());
    CHECK_EQUAL(Flags::WRITE_ONLY | Flags::CREATE, open.getFlags());
}

TEST(Touch, ClosesFileAndExitsAfterCreating)
{
    touch.start(host, "new.txt");

    filesystem.answerOpen(msg::Status::SUCCESS, { 5, 1 });

    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerClose();

    POINTERS_EQUAL(&touch, host.exited);
    CHECK_EQUAL(0, host.exitCode);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Touch, OpenFailureIsReported)
{
    touch.start(host, "new.txt");

    filesystem.answerOpen(msg::Status::NO_SPACE_LEFT);

    CHECK_TRUE(host.outputIs("touch: /new.txt: no space left\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Touch, TooLongPathIsRejected)
{
    etl::string<100> args(70, 'a');

    touch.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("touch: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Touch, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_CLOSE;

    touch.handle(message);

    CHECK_EQUAL(0, host.exitCount);
}
