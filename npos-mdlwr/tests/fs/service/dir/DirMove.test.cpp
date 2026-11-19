/**
 * @file DirMove.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/dir/Move.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(DirMove)
{
    mock::RecordingApi api;
    service::dir::Move move{api};
    mock::Filesystem filesystem{move, api};
    msg::dir::Move request{"/a", "/b"};

    void setup() override
    {
        move.initialize();
    }
};

TEST(DirMove, ChecksTheSourceFirst)
{
    move.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
}

TEST(DirMove, RenamesTheDirectoryWhenTheDestinationIsFree)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/b", filesystem.nextPath());

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);
    CHECK_EQUAL(fs::msg::Type::RENAME, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(0, api.responded.size());

    filesystem.answerRename();
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirMove, ExistingDestinationIsRejected)
{
    move.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::ALREADY_EXISTS, request.getStatus());
}

TEST(DirMove, MissingSourceIsReported)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(DirMove, FileAsSourceIsRejected)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, request.getStatus());
}

TEST(DirMove, MovingIntoItselfIsRejected)
{
    msg::dir::Move inside{"/a", "/a/b"};

    move.handleRequest(inside);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, inside.getStatus());
}

TEST(DirMove, SiblingWithTheSamePrefixIsNotInside)
{
    msg::dir::Move sibling{"/a", "/ab"};

    move.handleRequest(sibling);

    CHECK_TRUE(filesystem.hasRequest());
}

TEST(DirMove, RenameFailureIsReported)
{
    move.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    filesystem.answerRename(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(DirMove, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::dir::Move tooLong{"/a", etl::string_view(longPath.c_str(), longPath.size())};

    move.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
