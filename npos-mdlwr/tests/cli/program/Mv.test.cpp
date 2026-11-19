/**
 * @file Mv.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/cli/program/Mv.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Move.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(Mv)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::mdlwr::cli::program::Mv mv{api};
    mock::Filesystem filesystem{mv, api};

    // Starts the program and answers the checks of the destination and of the first source.
    void start(const char* args, fs::msg::Status destination, fs::Info::Type destinationType, fs::Info::Type source)
    {
        mv.start(host, args);
        filesystem.answerStat(destination, destinationType);
        filesystem.answerStat(fs::msg::Status::SUCCESS, source);
    }
};

TEST(Mv, HasNameAndDescription)
{
    CHECK_TRUE(mv.getName() == "mv");
    CHECK_TRUE(mv.getDescription().size() > 0);
}

TEST(Mv, WithoutEnoughOperandsPrintsUsage)
{
    mv.start(host, "a");

    CHECK_TRUE(host.outputIs("usage: mv <source>... <destination>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Mv, OptionsAreNotSupported)
{
    mv.start(host, "-r a b");

    CHECK_TRUE(host.outputIs("mv: invalid option -- 'r'\r\nusage: mv <source>... <destination>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Mv, MovesFile)
{
    start("a b", fs::msg::Status::NO_DIR_ENTRY, fs::Info::FILE, fs::Info::FILE);

    CHECK_EQUAL(msg::Type::FILE_MOVE, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    STRCMP_EQUAL("/b", filesystem.nextNewPath());
    filesystem.answerMiddleware<msg::file::Move>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Mv, MovesDirectoryWithoutAnyOption)
{
    start("x y", fs::msg::Status::NO_DIR_ENTRY, fs::Info::FILE, fs::Info::DIRECTORY);

    CHECK_EQUAL(msg::Type::DIR_MOVE, filesystem.nextType());
    STRCMP_EQUAL("/x", filesystem.nextPath());
    STRCMP_EQUAL("/y", filesystem.nextNewPath());
    filesystem.answerMiddleware<msg::dir::Move>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
}

TEST(Mv, MovesIntoExistingDirectoryUnderTheSameName)
{
    start("a/b d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, fs::Info::FILE);

    STRCMP_EQUAL("/d/b", filesystem.nextNewPath());
}

TEST(Mv, MovesManySourcesIntoDirectory)
{
    start("a b d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, fs::Info::FILE);
    filesystem.answerMiddleware<msg::file::Move>(msg::Status::SUCCESS);

    STRCMP_EQUAL("/b", filesystem.nextPath());
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    CHECK_EQUAL(msg::Type::DIR_MOVE, filesystem.nextType());
    STRCMP_EQUAL("/d/b", filesystem.nextNewPath());
    filesystem.answerMiddleware<msg::dir::Move>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Mv, ManySourcesNeedDirectoryAsDestination)
{
    mv.start(host, "a b c");
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);

    CHECK_TRUE(host.outputIs("mv: target 'c': not a directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Mv, MissingSourceIsReported)
{
    mv.start(host, "a b");
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("mv: cannot stat 'a': no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Mv, FailedMoveIsReportedAndTheRestIsMoved)
{
    start("x y d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, fs::Info::DIRECTORY);

    filesystem.answerMiddleware<msg::dir::Move>(msg::Status::ALREADY_EXISTS);

    CHECK_TRUE(host.outputIs("mv: cannot move 'x' to 'd': file exists\r\n"));
    STRCMP_EQUAL("/y", filesystem.nextPath());
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    filesystem.answerMiddleware<msg::file::Move>(msg::Status::SUCCESS);
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Mv, UnavailableServiceIsReported)
{
    start("a b", fs::msg::Status::NO_DIR_ENTRY, fs::Info::FILE, fs::Info::FILE);

    auto message = filesystem.take();
    message.output.status = npos::ipc::Message::STATUS_SERVICE_NOT_FOUND;
    mv.handleResponse(message);

    CHECK_TRUE(host.outputIs("mv: cannot move 'a' to 'b': service not available\r\n"));
}
