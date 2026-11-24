/**
 * @file DirCount.test.cpp
 * @author Adrian Szczepanski
 * @date 24-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/fs/service/dir/Count.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

TEST_GROUP(DirCount)
{
    mock::RecordingApi api;
    service::dir::Count count{api};
    mock::Filesystem filesystem{count, api};
    msg::dir::Count request{"/a"};

    void readEntries(size_t entries)
    {
        for(size_t i = 0; i < entries; i++)
            filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    }
};

TEST(DirCount, AcceptsOnlyItsRequest)
{
    CHECK_TRUE(count.acceptsRequest(msg::Type::DIR_COUNT));
    CHECK_FALSE(count.acceptsRequest(msg::Type::DIR_COPY));
}

TEST(DirCount, OpensTheDirectory)
{
    count.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::DIR_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(0, api.responded.size());
}

TEST(DirCount, ReadsFromTheOpenedDirectory)
{
    count.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    CHECK_EQUAL(fs::msg::Type::DIR_READ, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
}

TEST(DirCount, CountsEveryEntryFilesAndDirectoriesAlike)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "f");
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY, "d");
    filesystem.answerDirRead(fs::msg::Status::SUCCESS, fs::Info::FILE, "g");
    filesystem.answerEndOfDirectory();

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 1 }));
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(3, request.getCount());
}

TEST(DirCount, EmptyDirectoryHasNoEntries)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);

    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(0, request.getCount());
}

TEST(DirCount, MissingDirectoryIsReported)
{
    count.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, request.getStatus());
}

TEST(DirCount, FileInsteadOfDirectoryIsReported)
{
    count.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::NOT_A_DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, request.getStatus());
}

TEST(DirCount, LockedDirectoryIsBusy)
{
    count.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::LOCKED);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::BUSY, request.getStatus());
}

TEST(DirCount, OtherOpenErrorIsDeviceError)
{
    count.handleRequest(request);

    filesystem.answerDirOpen(fs::msg::Status::IO_ERROR);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
}

TEST(DirCount, ReadFailureIsDeviceErrorAfterClosingTheDirectory)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    readEntries(2);

    filesystem.answerDirRead(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(fs::msg::Type::DIR_CLOSE, filesystem.nextType());
    CHECK_EQUAL(0, api.responded.size());
    filesystem.answerDirClose();
    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
    CHECK_EQUAL(0, request.getCount());
}

TEST(DirCount, CloseFailureIsDeviceError)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    readEntries(2);
    filesystem.answerEndOfDirectory();

    filesystem.answerDirClose(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(msg::Status::DEVICE_ERROR, request.getStatus());
    CHECK_EQUAL(0, request.getCount());
}

TEST(DirCount, RequestWhileBusyIsAnsweredWithBusy)
{
    count.handleRequest(request);
    msg::dir::Count other{"/b"};

    count.handleRequest(other);

    CHECK_EQUAL(msg::Status::BUSY, other.getStatus());
    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(1, api.responded.size());
}

TEST(DirCount, BusyRequestDoesNotSpoilTheRunningOne)
{
    count.handleRequest(request);
    msg::dir::Count other{"/b"};
    count.handleRequest(other);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    readEntries(1);
    filesystem.answerEndOfDirectory();

    filesystem.answerDirClose();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(1, request.getCount());
}

TEST(DirCount, EveryRequestStartsCountingFromZero)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    readEntries(2);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();
    msg::dir::Count other{"/b"};

    count.handleRequest(other);
    filesystem.answerDirOpen(fs::msg::Status::SUCCESS);
    readEntries(1);
    filesystem.answerEndOfDirectory();
    filesystem.answerDirClose();

    CHECK_EQUAL(2, request.getCount());
    CHECK_EQUAL(1, other.getCount());
}

TEST(DirCount, TakesNextRequestAfterFailure)
{
    count.handleRequest(request);
    filesystem.answerDirOpen(fs::msg::Status::NO_DIR_ENTRY);
    msg::dir::Count other{"/b"};

    count.handleRequest(other);

    STRCMP_EQUAL("/b", filesystem.nextPath());
}

TEST(DirCount, IgnoresResponsesWithoutRequest)
{
    npos::ipc::Message message;
    message.type = fs::msg::Type::DIR_READ;

    count.handleResponse(message);

    CHECK_EQUAL(0, api.sent.size());
    CHECK_EQUAL(0, api.responded.size());
}

TEST(DirCount, IgnoresUnexpectedResponses)
{
    count.handleRequest(request);
    npos::ipc::Message message;
    message.type = fs::msg::Type::STAT;

    count.handleResponse(message);

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(0, api.responded.size());
}
