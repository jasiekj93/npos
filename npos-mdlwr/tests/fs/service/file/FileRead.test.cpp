/**
 * @file FileRead.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/file/Read.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(FileRead)
{
    mock::RecordingApi api;
    service::file::Read read{api};
    mock::Filesystem filesystem{read, api};
    uint8_t output[8] = {};
    msg::file::Read request{"/a", etl::span<uint8_t>(output)};

    void setup() override
    {
        read.initialize();
    }
};

TEST(FileRead, OpensTheFileForReading)
{
    read.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(fs::Flags::READ_ONLY, api.sent.front().input.open.flags);
}

TEST(FileRead, ReadsTheWholeFileIntoTheBuffer)
{
    read.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);
    CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
    CHECK_EQUAL(5, api.sent.front().output.size);
    filesystem.answerRead("hello");

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(5, request.getOutputLength());
    MEMCMP_EQUAL("hello", output, 5);
}

TEST(FileRead, FileFillingTheBufferExactlyFits)
{
    read.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::SUCCESS, 8);
    filesystem.answerRead("12345678");
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(8, request.getOutputLength());
}

TEST(FileRead, ContinuesAfterShortRead)
{
    read.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

    filesystem.answerRead("abc");

    CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
    CHECK_EQUAL(2, api.sent.front().output.size);
    filesystem.answerRead("de");
    filesystem.answerClose();

    CHECK_EQUAL(5, request.getOutputLength());
    MEMCMP_EQUAL("abcde", output, 5);
}

TEST(FileRead, EmptyFileIsNotRead)
{
    read.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::SUCCESS, 0);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(0, request.getOutputLength());
}

TEST(FileRead, TooSmallBufferIsReportedAndNothingIsRead)
{
    read.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::SUCCESS, 9);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::BUFFER_TOO_SMALL, request.getStatus());
    CHECK_EQUAL(0, request.getOutputLength());
}

TEST(FileRead, MissingFileIsReported)
{
    read.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::FILE_NOT_FOUND, request.getStatus());
}

TEST(FileRead, ReadFailureIsReportedAfterClosingTheFile)
{
    read.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

    filesystem.answerRead("", fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
    CHECK_EQUAL(0, request.getOutputLength());
}

TEST(FileRead, ShrunkFileEndsTheReadingWithWhatWasRead)
{
    read.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

    filesystem.answerRead("");
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(0, request.getOutputLength());
}

TEST(FileRead, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::file::Read tooLong{etl::string_view(longPath.c_str(), longPath.size()),
        etl::span<uint8_t>(output)};

    read.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}
