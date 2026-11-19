/**
 * @file Cp.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/cli/program/Cp.hpp>
#include <libnpos/mdlwr/fs/msg/dir/Copy.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(Cp)
{
    mock::RecordingApi api;
    mock::FakeHost host;
    npos::mdlwr::cli::program::Cp cp{api};
    mock::Filesystem filesystem{cp, api};

    void expect(npos::ipc::Message::Type type, const char* path)
    {
        CHECK_EQUAL(type, filesystem.nextType());
        STRCMP_EQUAL(path, filesystem.nextPath());
    }

    // Starts the program and answers the check of the destination.
    void start(const char* args, fs::msg::Status status, fs::Info::Type type = fs::Info::FILE)
    {
        cp.start(host, args);
        filesystem.answerStat(status, type);
    }

    void answerSource(fs::Info::Type type)
    {
        filesystem.answerStat(fs::msg::Status::SUCCESS, type);
    }
};

TEST(Cp, HasNameAndDescription)
{
    CHECK_TRUE(cp.getName() == "cp");
    CHECK_TRUE(cp.getDescription().size() > 0);
}

TEST(Cp, WithoutEnoughOperandsPrintsUsage)
{
    cp.start(host, "");
    CHECK_TRUE(host.outputIs("usage: cp [-r] <source>... <destination>\r\n"));
    CHECK_EQUAL(1, host.exitCode);

    host.output.clear();
    cp.start(host, "a");
    CHECK_TRUE(host.outputIs("usage: cp [-r] <source>... <destination>\r\n"));
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, UnknownOptionIsRejected)
{
    cp.start(host, "-x a b");

    CHECK_TRUE(host.outputIs("cp: invalid option -- 'x'\r\nusage: cp [-r] <source>... <destination>\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, ChecksTheDestinationFirst)
{
    cp.start(host, "a b");

    expect(fs::msg::Type::STAT, "/b");
}

TEST(Cp, CopiesFileToNewName)
{
    start("a b", fs::msg::Status::NO_DIR_ENTRY);
    expect(fs::msg::Type::STAT, "/a");
    answerSource(fs::Info::FILE);

    CHECK_EQUAL(msg::Type::FILE_COPY, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    STRCMP_EQUAL("/b", filesystem.nextNewPath());
    CHECK_EQUAL(0, host.exitCount);

    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Cp, OverwritesExistingFile)
{
    start("a b", fs::msg::Status::SUCCESS, fs::Info::FILE);
    answerSource(fs::Info::FILE);

    STRCMP_EQUAL("/b", filesystem.nextNewPath());
}

TEST(Cp, PathsAreRelativeToWorkingDirectory)
{
    host.cwd = "/work";
    start("a ../b", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::FILE);

    STRCMP_EQUAL("/work/a", filesystem.nextPath());
    STRCMP_EQUAL("/b", filesystem.nextNewPath());
}

TEST(Cp, CopiesIntoExistingDirectoryUnderTheSameName)
{
    start("x/y d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    answerSource(fs::Info::FILE);

    STRCMP_EQUAL("/x/y", filesystem.nextPath());
    STRCMP_EQUAL("/d/y", filesystem.nextNewPath());
}

TEST(Cp, DirectoryWithoutRecursiveOptionIsOmitted)
{
    start("x y", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::DIRECTORY);

    CHECK_TRUE(host.outputIs("cp: -r not specified; omitting directory 'x'\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, CopiesDirectoryWithRecursiveOption)
{
    start("-r x y", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::DIRECTORY);

    CHECK_EQUAL(msg::Type::DIR_COPY, filesystem.nextType());
    STRCMP_EQUAL("/x", filesystem.nextPath());
    STRCMP_EQUAL("/y", filesystem.nextNewPath());
    filesystem.answerMiddleware<msg::dir::Copy>(msg::Status::SUCCESS);
    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Cp, UpperCaseRecursiveOptionIsTheSame)
{
    start("-R x y", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::DIRECTORY);

    CHECK_EQUAL(msg::Type::DIR_COPY, filesystem.nextType());
}

TEST(Cp, RecursiveOptionStillCopiesFilesAsFiles)
{
    start("-r a b", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::FILE);

    CHECK_EQUAL(msg::Type::FILE_COPY, filesystem.nextType());
}

TEST(Cp, DoubleDashEndsTheOptions)
{
    start("-- -a b", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::FILE);

    STRCMP_EQUAL("/-a", filesystem.nextPath());
}

TEST(Cp, CopiesManySourcesIntoDirectoryOneAfterAnother)
{
    start("a b d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    answerSource(fs::Info::FILE);
    STRCMP_EQUAL("/d/a", filesystem.nextNewPath());
    CHECK_EQUAL(1, api.sent.size());
    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::SUCCESS);

    expect(fs::msg::Type::STAT, "/b");
    answerSource(fs::Info::FILE);
    STRCMP_EQUAL("/d/b", filesystem.nextNewPath());
    CHECK_EQUAL(0, host.exitCount);
    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Cp, ManySourcesNeedDirectoryAsDestination)
{
    start("a b c", fs::msg::Status::SUCCESS, fs::Info::FILE);

    CHECK_TRUE(host.outputIs("cp: target 'c': not a directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, ManySourcesNeedExistingDestination)
{
    start("a b c", fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("cp: target 'c': no such file or directory\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Cp, MissingSourceIsReportedAndTheRestIsCopied)
{
    start("a b d", fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.outputIs("cp: cannot stat 'a': no such file or directory\r\n"));
    expect(fs::msg::Type::STAT, "/b");
    answerSource(fs::Info::FILE);
    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::SUCCESS);

    CHECK_EQUAL(1, host.exitCode);
    CHECK_EQUAL(1, host.exitCount);
}

TEST(Cp, FailedCopyIsReported)
{
    start("a b", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::FILE);

    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::NO_SPACE_LEFT);

    CHECK_TRUE(host.outputIs("cp: cannot copy 'a' to 'b': no space left\r\n"));
    CHECK_EQUAL(1, host.exitCode);
}

TEST(Cp, FailedDestinationCheckIsReported)
{
    start("a b", fs::msg::Status::IO_ERROR);

    CHECK_TRUE(host.outputIs("cp: cannot stat 'b': I/O error\r\n"));
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, TooLongSourceNameIsReported)
{
    etl::string<100> args(70, 'a');
    args.append(" b");
    start(args.c_str(), fs::msg::Status::NO_DIR_ENTRY);

    CHECK_TRUE(host.output.find("name too long") != etl::string<2048>::npos);
    CHECK_EQUAL(1, host.exitCode);
    CHECK_FALSE(filesystem.hasRequest());
}

TEST(Cp, CanBeStartedAgain)
{
    start("a b", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::FILE);
    filesystem.answerMiddleware<msg::file::Copy>(msg::Status::NO_SPACE_LEFT);
    host.output.clear();

    start("-r c d", fs::msg::Status::NO_DIR_ENTRY);
    answerSource(fs::Info::DIRECTORY);
    filesystem.answerMiddleware<msg::dir::Copy>(msg::Status::SUCCESS);

    CHECK_EQUAL(0, host.exitCode);
    CHECK_TRUE(host.outputIs(""));
}

TEST(Cp, IgnoresUnexpectedResponses)
{
    npos::ipc::Message message;
    message.type = msg::Type::FILE_COPY;
    cp.handleResponse(message);

    start("a b", fs::msg::Status::NO_DIR_ENTRY);
    message.type = fs::msg::Type::DIR_OPEN;
    cp.handleResponse(message);

    CHECK_EQUAL(0, host.exitCount);
    CHECK_EQUAL(1, api.sent.size());
}
