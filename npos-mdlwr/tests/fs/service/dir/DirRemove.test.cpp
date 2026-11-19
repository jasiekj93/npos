/**
 * @file DirRemove.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/dir/Remove.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(DirRemove)
{
    mock::RecordingApi api;
    service::dir::Remove remove{api};
    mock::Filesystem filesystem{remove, api};
    msg::dir::Remove request{npos::ipc::NULL_OID, "/d"};

    void setup() override
    {
        remove.initialize();
    }

    void expect(npos::ipc::Message::Type type, const char* path)
    {
        CHECK_EQUAL(type, filesystem.nextType());
        STRCMP_EQUAL(path, filesystem.nextPath());
    }
};

TEST(DirRemove, RemovesEmptyDirectory)
{
    remove.handleRequest(request);

    expect(fs::msg::Type::DIR_OPEN, "/d");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    CHECK_EQUAL(fs::msg::Type::DIR_READ, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
    filesystem.answerEndOfDirectory();

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
    filesystem.answerDirClose();

    expect(fs::msg::Type::DIR_REMOVE, "/d");
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirRemove();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirRemove, RemovesFilesBeforeTheDirectory)
{
    remove.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    filesystem.answerDirClose();
    expect(fs::msg::Type::FILE_REMOVE, "/d/f");
    filesystem.answerFileRemove();

    expect(fs::msg::Type::DIR_OPEN, "/d");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    expect(fs::msg::Type::DIR_REMOVE, "/d");
    filesystem.answerDirRemove();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirRemove, RemovesNestedDirectories)
{
    remove.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "s");
    filesystem.answerDirClose();
    expect(fs::msg::Type::DIR_OPEN, "/d/s");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    filesystem.answerDirClose();
    expect(fs::msg::Type::FILE_REMOVE, "/d/s/f");
    filesystem.answerFileRemove();

    expect(fs::msg::Type::DIR_OPEN, "/d/s");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    expect(fs::msg::Type::DIR_REMOVE, "/d/s");
    filesystem.answerDirRemove();

    expect(fs::msg::Type::DIR_OPEN, "/d");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    expect(fs::msg::Type::DIR_REMOVE, "/d");
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirRemove();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirRemove, MissingDirectoryIsReported)
{
    remove.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(DirRemove, FileIsNotRemoved)
{
    remove.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::NOT_A_DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, request.getStatus());
}

TEST(DirRemove, RootDirectoryIsRejected)
{
    msg::dir::Remove root{npos::ipc::NULL_OID, "/"};

    remove.handleRequest(root);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, root.getStatus());
}

TEST(DirRemove, ReadFailureIsReportedAfterClosingTheDirectory)
{
    remove.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    filesystem.answerDirClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(DirRemove, FileRemovalFailureIsReported)
{
    remove.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    filesystem.answerDirClose();

    filesystem.answerFileRemove(fs::msg::Status::IO_ERROR);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(DirRemove, DirectoryRemovalFailureIsReported)
{
    remove.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();

    filesystem.answerDirRemove(fs::msg::Status::DIRECTORY_NOT_EMPTY);

    CHECK_EQUAL(msg::Status::ALREADY_EXISTS, request.getStatus());
}

TEST(DirRemove, TooLongEntryPathIsReportedAfterClosingTheDirectory)
{
    etl::string<100> name(30, 'a');
    etl::string<100> path(60, 'b');
    path.insert(0, "/");
    msg::dir::Remove deep{npos::ipc::NULL_OID, etl::string_view(path.c_str(), path.size())};
    remove.handleRequest(deep);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, name.c_str());
    filesystem.answerDirClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, deep.getStatus());
}

TEST(DirRemove, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::dir::Remove tooLong{npos::ipc::NULL_OID, etl::string_view(longPath.c_str(), longPath.size())};

    remove.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
