/**
 * @file DirCopy.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/dir/Copy.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(DirCopy)
{
    mock::RecordingApi api;
    static constexpr npos::ipc::Oid FILE_COPY{ 3, 7 };
    service::dir::Copy copy{api, FILE_COPY};
    mock::Filesystem filesystem{copy, api};
    msg::dir::Copy request{npos::ipc::NULL_OID, "/a", "/b"};

    void setup() override
    {
        copy.initialize();
    }

    void expect(npos::ipc::Message::Type type, const char* path)
    {
        CHECK_EQUAL(type, filesystem.nextType());
        STRCMP_EQUAL(path, filesystem.nextPath());
    }

    // Takes the request through the checks and the creation of the destination up to reading the source.
    void startCopying()
    {
        copy.handleRequest(request);
        filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
        filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);
        filesystem.answerDirMake();
        filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    }
};

TEST(DirCopy, ChecksTheSourceThenTheDestinationAndCreatesIt)
{
    copy.handleRequest(request);
    expect(fs::msg::Type::STAT, "/a");
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    expect(fs::msg::Type::STAT, "/b");
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    expect(fs::msg::Type::DIR_MAKE, "/b");
    filesystem.answerDirMake();

    expect(fs::msg::Type::DIR_OPEN, "/a");
}

TEST(DirCopy, EmptyDirectoryIsDoneWhenCreated)
{
    startCopying();

    CHECK_EQUAL(fs::msg::Type::DIR_READ, filesystem.nextType());
    filesystem.answerEndOfDirectory();
    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirCopy, FilesAreCopiedByTheGivenFileCopyService)
{
    startCopying();

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");

    CHECK_EQUAL(msg::Type::FILE_COPY, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == FILE_COPY);
    auto message = api.sent.front();
    auto& fileCopy = reinterpret_cast<msg::file::Copy&>(message);
    STRCMP_EQUAL("/a/f", fileCopy.getOldPath().data());
    STRCMP_EQUAL("/b/f", fileCopy.getNewPath().data());
    filesystem.answerFileCopy(msg::Status::SUCCESS);

    CHECK_EQUAL(fs::msg::Type::DIR_READ, filesystem.nextType());
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirCopy, ConsecutiveEntriesAreAllCopied)
{
    startCopying();

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    filesystem.answerFileCopy(msg::Status::SUCCESS);
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "g");

    expect(msg::Type::FILE_COPY, "/a/g");
}

TEST(DirCopy, SubdirectoryIsCopiedAndTheListingContinuesAfterIt)
{
    startCopying();

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "s");
    filesystem.answerDirClose();
    expect(fs::msg::Type::DIR_MAKE, "/b/s");
    filesystem.answerDirMake();
    expect(fs::msg::Type::DIR_OPEN, "/a/s");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "g");
    expect(msg::Type::FILE_COPY, "/a/s/g");
    filesystem.answerFileCopy(msg::Status::SUCCESS);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();

    // Back in the parent: the reopened directory lists "s" again, which is skipped.
    expect(fs::msg::Type::DIR_OPEN, "/a");
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "s");
    CHECK_EQUAL(fs::msg::Type::DIR_READ, filesystem.nextType());
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "t");
    expect(msg::Type::FILE_COPY, "/a/t");
    filesystem.answerFileCopy(msg::Status::SUCCESS);

    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(DirCopy, ExistingDestinationIsRejected)
{
    copy.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::ALREADY_EXISTS, request.getStatus());
}

TEST(DirCopy, MissingSourceIsReported)
{
    copy.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(DirCopy, FileAsSourceIsRejected)
{
    copy.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, request.getStatus());
}

TEST(DirCopy, CopyingIntoItselfIsRejected)
{
    msg::dir::Copy inside{npos::ipc::NULL_OID, "/a", "/a/b"};

    copy.handleRequest(inside);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, inside.getStatus());
}

TEST(DirCopy, MissingParentOfTheDestinationIsReported)
{
    copy.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    filesystem.answerDirMake(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(DirCopy, FailedFileCopyIsReportedAfterClosingTheDirectory)
{
    startCopying();
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");

    filesystem.answerFileCopy(msg::Status::NO_SPACE_LEFT);

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NO_SPACE_LEFT, request.getStatus());
}

TEST(DirCopy, ReadFailureIsReportedAfterClosingTheDirectory)
{
    startCopying();

    filesystem.answerDirRead(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    filesystem.answerDirClose();
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(DirCopy, TooLongEntryPathIsReportedAfterClosingTheDirectory)
{
    etl::string<100> name(40, 'a');
    etl::string<100> path(30, 'b');
    path.insert(0, "/");
    msg::dir::Copy deep{npos::ipc::NULL_OID, etl::string_view(path.c_str(), path.size()), "/b"};
    copy.handleRequest(deep);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);
    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);
    filesystem.answerDirMake();
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, name.c_str());
    filesystem.answerDirClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, deep.getStatus());
}

TEST(DirCopy, TooDeepTreeIsReported)
{
    startCopying();

    for(size_t level = 0; level < service::dir::Copy::MAX_DEPTH - 1; level++)
    {
        filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "d");
        filesystem.answerDirClose();
        filesystem.answerDirMake();
        filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    }

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "d");
    filesystem.answerDirClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, request.getStatus());
}

TEST(DirCopy, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::dir::Copy tooLong{npos::ipc::NULL_OID, etl::string_view(longPath.c_str(), longPath.size()), "/b"};

    copy.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
