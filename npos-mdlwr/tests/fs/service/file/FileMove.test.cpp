/**
 * @file FileMove.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/file/Move.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(FileMove)
{
    mock::RecordingApi api;
    service::file::Move move{api};
    mock::Filesystem filesystem{move, api};
    msg::file::Move request{npos::ipc::NULL_OID, "/a", "/b"};

    void setup() override
    {
        move.initialize();
    }
};

TEST(FileMove, ChecksTheSourceFirst)
{
    move.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
}

TEST(FileMove, RemovesTheDestinationAndRenamesTheSource)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    CHECK_EQUAL(fs::msg::Type::FILE_REMOVE, filesystem.nextType());
    STRCMP_EQUAL("/b", filesystem.nextPath());

    filesystem.answerFileRemove();
    CHECK_EQUAL(fs::msg::Type::RENAME, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(0, api.responded.size());

    filesystem.answerRename();
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(FileMove, MissingDestinationIsNotAnError)
{
    move.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS);

    filesystem.answerFileRemove(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_EQUAL(fs::msg::Type::RENAME, filesystem.nextType());
}

TEST(FileMove, MissingSourceKeepsTheDestination)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::FILE_NOT_FOUND, request.getStatus());
}

TEST(FileMove, DirectoryAsSourceIsRejected)
{
    move.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_FILE, request.getStatus());
}

TEST(FileMove, DirectoryAsDestinationIsRejected)
{
    move.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS);

    filesystem.answerFileRemove(fs::msg::Status::IS_A_DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_FILE, request.getStatus());
}

TEST(FileMove, RenameFailureIsReported)
{
    move.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS);
    filesystem.answerFileRemove();

    filesystem.answerRename(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(FileMove, MoveOntoItselfLeavesTheFileUntouched)
{
    msg::file::Move same{npos::ipc::NULL_OID, "/a", "/a"};
    move.handleRequest(same);

    filesystem.answerStat(fs::msg::Status::SUCCESS);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::SUCCESS, same.getStatus());
}

TEST(FileMove, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::file::Move tooLong{npos::ipc::NULL_OID, etl::string_view(longPath.c_str(), longPath.size()), "/b"};

    move.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
