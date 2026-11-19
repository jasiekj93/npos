/**
 * @file Rm.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/cli/program/Rm.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Remove.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(Rm)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::mdlwr::cli::program::Rm rm{api};
    mock::Filesystem filesystem{rm, api};

    void start(const char* args, fs::Info::Type type)
    {
        rm.start(host, args);
        filesystem.answerStat(fs::msg::Status::SUCCESS, type);
    }
};

TEST(Rm, HasNameAndDescription)
{
    CHECK_TRUE(rm.getName() == "rm");
    CHECK_TRUE(rm.getDescription().size() > 0);
}

TEST(Rm, WithoutOperandsPrintsUsage)
{
    rm.start(host, "-r");

    CHECK_TRUE(host.outputIs("usage: rm [-rf] <path>...\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Rm, UnknownOptionIsRejected)
{
    rm.start(host, "-x a");

    CHECK_TRUE(host.outputIs("rm: invalid option -- 'x'\r\nusage: rm [-rf] <path>...\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Rm, ChecksWhatThePathIs)
{
    host.cwd = "/work";

    rm.start(host, "a");

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/work/a", filesystem.nextPath());
}

TEST(Rm, RemovesFile)
{
    start("a", fs::Info::FILE);

    CHECK_EQUAL(fs::msg::Type::FILE_REMOVE, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    filesystem.answerFileRemove();

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Rm, DirectoryWithoutRecursiveOptionIsRefused)
{
    start("d", fs::Info::DIRECTORY);

    CHECK_TRUE(host.outputIs("rm: cannot remove 'd': is a directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Rm, RemovesDirectoryWithRecursiveOption)
{
    start("-r d", fs::Info::DIRECTORY);

    CHECK_EQUAL(msg::Type::DIR_REMOVE, filesystem.nextType());
    STRCMP_EQUAL("/d", filesystem.nextPath());
    filesystem.answerMiddleware<msg::dir::Remove>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Rm, RecursiveOptionStillRemovesFilesAsFiles)
{
    start("-R a", fs::Info::FILE);

    CHECK_EQUAL(fs::msg::Type::FILE_REMOVE, filesystem.nextType());
}

TEST(Rm, MissingPathIsReported)
{
    rm.start(host, "a");

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("rm: cannot remove 'a': no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Rm, ForceIgnoresMissingPath)
{
    rm.start(host, "-f a");

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs(""));
    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Rm, ForceDoesNotHideOtherErrors)
{
    rm.start(host, "-f a");

    filesystem.answerStat(fs::msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("rm: cannot remove 'a': I/O error\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Rm, OptionsCanBeCombined)
{
    rm.start(host, "-rf a");
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    filesystem.answerMiddleware<msg::dir::Remove>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);

    rm.start(host, "-r -f b");
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);
    CHECK_EQUAL(0, host.exitCode);
}

TEST(Rm, RemovesManyPathsOneAfterAnother)
{
    start("a b", fs::Info::FILE);
    filesystem.answerFileRemove();

    STRCMP_EQUAL("/b", filesystem.nextPath());
    CHECK_EQUAL(0, host.exitCount);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    filesystem.answerFileRemove();

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Rm, FailureIsReportedAndTheRestIsRemoved)
{
    start("a b", fs::Info::FILE);

    filesystem.answerFileRemove(fs::msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("rm: cannot remove 'a': I/O error\r\n"));
    STRCMP_EQUAL("/b", filesystem.nextPath());
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    filesystem.answerFileRemove();
    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Rm, FailedDirectoryRemovalIsReported)
{
    start("-r /", fs::Info::DIRECTORY);

    filesystem.answerMiddleware<msg::dir::Remove>(msg::Status::INVALID_PATH);

    CHECK_TRUE(host.outputIs("rm: cannot remove '/': invalid path\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Rm, TooLongNameIsReported)
{
    etl::string<100> args(70, 'a');

    rm.start(host, args.c_str());

    CHECK_TRUE(host.output.find("name too long") != etl::string<2048>::npos);
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Rm, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = fs::msg::Type::FILE_REMOVE;
    rm.handleResponse(message);

    rm.start(host, "a");
    rm.handleResponse(message);

    CHECK_EQUAL(0, host.exitCount);
}
