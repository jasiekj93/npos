/**
 * @file Cat.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/ProgramHarness.hpp"

#include <libnpos/cli/program/Cat.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Cat)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::cli::program::Cat cat{api};
    mock::ProgramHarness filesystem{cat, api};

    // Starts cat on a file that opens successfully.
    void startOpened(const char* args = "a.txt")
    {
        cat.start(host, args);
        filesystem.answerOpen(msg::Status::SUCCESS, { 5, 2 });
    }
};

TEST(Cat, HasNameAndDescription)
{
    CHECK_TRUE(cat.getName() == "cat");
    CHECK_TRUE(cat.getDescription().size() > 0);
}

TEST(Cat, WithoutArgumentsPrintsUsage)
{
    cat.start(host, "  ");

    CHECK_TRUE(host.outputIs("usage: cat <file>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cat, OpensFileRelativeToWorkingDirectory)
{
    host.cwd = "/dir";

    cat.start(host, "a.txt");

    auto request = filesystem.take();
    CHECK_EQUAL(msg::Type::FILE_OPEN, request.type);
    auto& open = reinterpret_cast<msg::file::Open&>(request);
    STRCMP_EQUAL("/dir/a.txt", open.getPath().data());
    CHECK_EQUAL(Flags::READ_ONLY, open.getFlags());
}

TEST(Cat, AbsolutePathIgnoresWorkingDirectory)
{
    host.cwd = "/dir";

    cat.start(host, "/other/b.txt");

    auto request = filesystem.take();
    STRCMP_EQUAL("/other/b.txt", reinterpret_cast<msg::file::Open&>(request).getPath().data());
}

TEST(Cat, TooLongPathIsRejected)
{
    etl::string<100> args(70, 'a');

    cat.start(host, etl::string_view(args.c_str(), args.size()));

    CHECK_TRUE(host.outputIs("cat: path too long\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cat, OpenFailureIsReportedWithoutClosing)
{
    cat.start(host, "a.txt");

    filesystem.answerOpen(msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("cat: /a.txt: no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cat, ReadsFromTheOpenedFile)
{
    startOpened();

    CHECK_EQUAL(msg::Type::FILE_READ, filesystem.nextType());
    auto request = filesystem.take();
    CHECK_EQUAL(2, request.recipient.serviceId);
    CHECK_EQUAL(64, request.input.readWrite.length);
}

TEST(Cat, PrintsFileContentUntilEndOfFile)
{
    startOpened();

    filesystem.answerRead("hello ");
    filesystem.answerRead("world");
    filesystem.answerRead("");

    CHECK_TRUE(host.outputIs("hello world"));
    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerClose();

    POINTERS_EQUAL(&cat, host.exited);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Cat, ReadFailureIsReportedAndFileClosed)
{
    startOpened();

    filesystem.answerRead("", msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("cat: /a.txt: I/O error\r\n"));
    CHECK_EQUAL(msg::Type::FILE_CLOSE, filesystem.nextType());

    filesystem.answerClose();

    CHECK_EQUAL(1, host.exitCode);
}

TEST(Cat, WaitsForOutputToDrainAndContinuesOnResume)
{
    startOpened();
    host.capacity = 3;

    filesystem.answerRead("hello");

    CHECK_TRUE(host.outputIs("hel"));
    CHECK_FALSE(filesystem.hasRequest());

    host.capacity = 100;
    cat.resume();

    CHECK_TRUE(host.outputIs("hello"));
    CHECK_EQUAL(msg::Type::FILE_READ, filesystem.nextType());
}

TEST(Cat, ResumeWithoutPendingOutputDoesNothing)
{
    startOpened();

    cat.resume();

    CHECK_EQUAL(1, api.sent.size());
}

TEST(Cat, IgnoresResponsesOutOfOrder)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_READ;
    message.output.status = msg::Status::SUCCESS;
    message.output.readWrite.length = 5;

    cat.handle(message);

    CHECK_TRUE(host.outputIs(""));
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cat, IgnoresUnrelatedMessageTypes)
{
    startOpened();
    filesystem.take();
    npos::ipc::Message message;
    message.type = npos::ipc::DEVICE_MESSAGE_TYPE;

    cat.handle(message);

    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cat, CanBeRunAgainAfterFinishing)
{
    startOpened();
    filesystem.answerRead("");
    filesystem.answerClose();
    host.output.clear();

    startOpened("b.txt");
    filesystem.answerRead("x");

    CHECK_TRUE(host.outputIs("x"));
}
