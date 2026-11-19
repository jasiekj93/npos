/**
 * @file FileWrite.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/file/Write.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(FileWrite)
{
    mock::RecordingApi api;
    service::file::Write write{api};
    mock::Filesystem filesystem{write, api};
    const uint8_t input[5] = { 'h', 'e', 'l', 'l', 'o' };
    msg::file::Write request{npos::ipc::NULL_OID, "/a", etl::span<const uint8_t>(input), false};

    void setup() override
    {
        write.initialize();
    }
};

TEST(FileWrite, OpensTheFileToOverwriteIt)
{
    write.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(fs::Flags::WRITE_ONLY | fs::Flags::CREATE | fs::Flags::TRUNCATE, api.sent.front().input.open.flags);
}

TEST(FileWrite, OpensTheFileToAppend)
{
    msg::file::Write append{npos::ipc::NULL_OID, "/a", etl::span<const uint8_t>(input), true};

    write.handleRequest(append);

    CHECK_EQUAL(fs::Flags::WRITE_ONLY | fs::Flags::CREATE | fs::Flags::APPEND, api.sent.front().input.open.flags);
}

TEST(FileWrite, WritesTheWholeInputAndClosesTheFile)
{
    write.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    CHECK_EQUAL(fs::msg::Type::FILE_WRITE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 0 }));
    MEMCMP_EQUAL("hello", api.sent.front().input.data, 5);
    filesystem.answerWrite();

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerClose();

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(FileWrite, ContinuesAfterShortWrite)
{
    write.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    filesystem.answerWrite(fs::msg::Status::SUCCESS, 2);

    CHECK_EQUAL(fs::msg::Type::FILE_WRITE, filesystem.nextType());
    CHECK_EQUAL(3, api.sent.front().input.size);
    MEMCMP_EQUAL("llo", api.sent.front().input.data, 3);
    filesystem.answerWrite();
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(FileWrite, EmptyInputOnlyCreatesTheFile)
{
    msg::file::Write empty{npos::ipc::NULL_OID, "/a", etl::span<const uint8_t>(), false};
    write.handleRequest(empty);

    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_EQUAL(msg::Status::SUCCESS, empty.getStatus());
}

TEST(FileWrite, MissingDirectoryIsReported)
{
    write.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(FileWrite, DirectoryAsFileIsReported)
{
    write.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::IS_A_DIRECTORY);

    CHECK_EQUAL(msg::Status::NOT_A_FILE, request.getStatus());
}

TEST(FileWrite, FullFilesystemIsReportedAfterClosingTheFile)
{
    write.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    filesystem.answerWrite(fs::msg::Status::NO_SPACE_LEFT);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_EQUAL(msg::Status::NO_SPACE_LEFT, request.getStatus());
}

TEST(FileWrite, WriteAcceptingNothingDoesNotLoopForever)
{
    write.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    filesystem.answerWrite(fs::msg::Status::SUCCESS, 0);
    filesystem.answerClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NO_SPACE_LEFT, request.getStatus());
}

TEST(FileWrite, FailedCloseTurnsSuccessIntoError)
{
    write.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);
    filesystem.answerWrite();

    filesystem.answerClose(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(FileWrite, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::file::Write tooLong{npos::ipc::NULL_OID, etl::string_view(longPath.c_str(), longPath.size()),
        etl::span<const uint8_t>(input), false};

    write.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
