/**
 * @file FileCopy.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/file/Copy.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(FileCopy)
{
    mock::RecordingApi api;
    uint8_t buffer[4];
    service::file::Copy copy{api, etl::span<uint8_t>(buffer)};
    mock::Filesystem filesystem{copy, api};
    msg::file::Copy request{npos::ipc::NULL_OID, "/a", "/b"};

    void setup() override
    {
        copy.initialize();
    }

    void openBoth()
    {
        copy.handleRequest(request);
        filesystem.answerOpen(fs::msg::Status::SUCCESS, 0, { 5, 0 });
        filesystem.answerOpen(fs::msg::Status::SUCCESS, 0, { 5, 1 });
    }
};

TEST(FileCopy, AcceptsOnlyItsRequest)
{
    CHECK_TRUE(copy.acceptsRequest(msg::Type::FILE_COPY));
    CHECK_FALSE(copy.acceptsRequest(msg::Type::FILE_MOVE));
}

TEST(FileCopy, OpensTheSourceForReading)
{
    copy.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(fs::Flags::READ_ONLY, api.sent.front().input.open.flags);
}

TEST(FileCopy, OpensTheDestinationForOverwriting)
{
    copy.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    STRCMP_EQUAL("/b", filesystem.nextPath());
    CHECK_EQUAL(fs::Flags::WRITE_ONLY | fs::Flags::CREATE | fs::Flags::TRUNCATE, api.sent.front().input.open.flags);
}

TEST(FileCopy, MovesTheDataThroughTheBufferAndClosesBothFiles)
{
    openBoth();

    CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 0 }));
    filesystem.answerRead("abcd");

    CHECK_EQUAL(fs::msg::Type::FILE_WRITE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
    MEMCMP_EQUAL("abcd", api.sent.front().input.data, 4);
    filesystem.answerWrite();

    filesystem.answerRead("ef");
    CHECK_EQUAL(2, api.sent.front().input.size);
    filesystem.answerWrite();

    filesystem.answerRead("");
    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
    filesystem.answerClose();
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 0 }));
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerClose();

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}

TEST(FileCopy, MissingSourceIsReported)
{
    copy.handleRequest(request);

    filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::FILE_NOT_FOUND, request.getStatus());
}

TEST(FileCopy, MissingDestinationDirectoryIsReportedAfterClosingTheSource)
{
    copy.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerClose();
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(FileCopy, DirectoryAsDestinationIsReported)
{
    copy.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::SUCCESS);
    filesystem.answerOpen(fs::msg::Status::IS_A_DIRECTORY);
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::NOT_A_FILE, request.getStatus());
}

TEST(FileCopy, FullFilesystemIsReportedAfterClosingBothFiles)
{
    openBoth();
    filesystem.answerRead("abcd");

    filesystem.answerWrite(fs::msg::Status::NO_SPACE_LEFT);
    filesystem.answerClose();
    filesystem.answerClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NO_SPACE_LEFT, request.getStatus());
}

TEST(FileCopy, ShortWriteIsReportedAsFullFilesystem)
{
    openBoth();
    filesystem.answerRead("abcd");

    filesystem.answerWrite(fs::msg::Status::SUCCESS, 2);
    filesystem.answerClose();
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::NO_SPACE_LEFT, request.getStatus());
}

TEST(FileCopy, ReadFailureIsReportedAfterClosingBothFiles)
{
    openBoth();

    filesystem.answerRead("", fs::msg::Status::IO_ERROR);
    filesystem.answerClose();
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(FileCopy, FailedCloseTurnsSuccessIntoError)
{
    openBoth();
    filesystem.answerRead("");

    filesystem.answerClose(fs::msg::Status::IO_ERROR);
    filesystem.answerClose();

    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(FileCopy, CopyOntoItselfLeavesTheFileUntouched)
{
    msg::file::Copy same{npos::ipc::NULL_OID, "/a", "/a"};
    copy.handleRequest(same);

    filesystem.answerOpen(fs::msg::Status::SUCCESS);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    filesystem.answerClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::SUCCESS, same.getStatus());
}

TEST(FileCopy, TooLongPathIsRejected)
{
    etl::string<100> longPath(70, 'a');
    msg::file::Copy tooLong{npos::ipc::NULL_OID, "/a", etl::string_view(longPath.c_str(), longPath.size())};

    copy.handleRequest(tooLong);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::INVALID_PATH, tooLong.getStatus());
}

TEST(FileCopy, RequestWhileBusyIsAnsweredWithBusy)
{
    copy.handleRequest(request);
    msg::file::Copy other{npos::ipc::NULL_OID, "/c", "/d"};

    copy.handleRequest(other);

    CHECK_EQUAL(msg::Status::BUSY, other.getStatus());
    CHECK_EQUAL(1, api.sent.size());
}

TEST(FileCopy, TakesNextRequestWhenDone)
{
    copy.handleRequest(request);
    filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);
    msg::file::Copy other{npos::ipc::NULL_OID, "/c", "/d"};

    copy.handleRequest(other);

    STRCMP_EQUAL("/c", filesystem.nextPath());
}

TEST(FileCopy, IgnoresUnexpectedResponses)
{
    copy.handleRequest(request);
    npos::ipc::Message message;
    message.type = fs::msg::Type::FILE_READ;

    copy.handleResponse(message);

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(1, api.sent.size());
}
