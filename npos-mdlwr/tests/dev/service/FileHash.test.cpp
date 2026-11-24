/**
 * @file FileHash.test.cpp
 * @author Adrian Szczepanski
 * @date 24-11-2026
 */

#include "mock/Filesystem.hpp"

#include <libnpos/mdlwr/dev/service/FileHash.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::dev;
namespace fs = npos::fs;
namespace device = npos::dev;

TEST_GROUP(FileHash)
{
    static constexpr npos::ipc::Oid HASH{ 6, 2 };
    static constexpr npos::ipc::Oid FILE{ 5, 0 };

    mock::RecordingApi api;
    uint8_t chunk[4];
    service::FileHash fileHash{api, "hash0", etl::span<uint8_t>(chunk)};
    mock::Filesystem filesystem{fileHash, api};
    uint8_t output[32] = {};
    msg::FileHash request{"/a", etl::span<uint8_t>(output)};

    void answerDeviceOpen(device::msg::Open::Status status, npos::ipc::Oid opened = HASH)
    {
        auto message = filesystem.take();
        reinterpret_cast<device::msg::Open&>(message).setStatus(status).setOpened(opened);
        fileHash.handleResponse(message);
    }

    // The timer is sent right after the open request of the device and nobody answers it by itself.
    void dropTimer()
    {
        CHECK_EQUAL(device::msg::Type::TIMER, filesystem.nextType());
        filesystem.take();
    }

    void fireTimer(device::msg::Timer::Status status = device::msg::Timer::SUCCESS)
    {
        npos::ipc::Message message;
        message.type = device::msg::Type::TIMER;
        reinterpret_cast<device::msg::Timer&>(message).setStatus(status);
        fileHash.handleResponse(message);
    }

    void answerHash(device::msg::Hash::Status status = device::msg::Hash::SUCCESS)
    {
        auto message = filesystem.take();
        auto& response = reinterpret_cast<device::msg::Hash&>(message);

        if(response.getOperation() == device::msg::Hash::ACCUMULATE_END)
            std::memset(response.getOutput().data(), 0xAB, response.getOutput().size());

        response.setStatus(status);
        fileHash.handleResponse(message);
    }

    void answerDeviceClose(device::msg::Close::Status status = device::msg::Close::SUCCESS)
    {
        auto message = filesystem.take();
        reinterpret_cast<device::msg::Close&>(message).setStatus(status);
        fileHash.handleResponse(message);
    }

    // Takes the request through the check of the file, up to the open request of the device.
    void startOpeningDevice()
    {
        fileHash.handleRequest(request);
        filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    }

    // Takes the request through opening of the device and the file, up to the first read.
    void startReading(size_t fileSize)
    {
        startOpeningDevice();
        answerDeviceOpen(device::msg::Open::SUCCESS);
        dropTimer();
        filesystem.answerOpen(fs::msg::Status::SUCCESS, fileSize, FILE);
    }

    // Reads and hashes the whole file of the given content, up to the closing of the file and the device.
    void hashFile(const char* content)
    {
        startReading(std::strlen(content));
        filesystem.answerRead(content);
        answerHash();
    }
};

TEST(FileHash, AcceptsOnlyItsRequest)
{
    CHECK_TRUE(fileHash.acceptsRequest(msg::Type::FILE_HASH));
    CHECK_FALSE(fileHash.acceptsRequest(msg::Type::UNUSED));
}

TEST(FileHash, ChecksTheFileFirst)
{
    fileHash.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(0, api.responded.size());
}

TEST(FileHash, MissingFileIsReported)
{
    fileHash.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::NO_DIR_ENTRY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::FileHash::FILE_NOT_FOUND, request.getStatus());
}

TEST(FileHash, DirectoryIsReported)
{
    fileHash.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::DIRECTORY);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::FileHash::IS_A_DIRECTORY, request.getStatus());
}

TEST(FileHash, OtherCheckErrorIsDeviceError)
{
    fileHash.handleRequest(request);

    filesystem.answerStat(fs::msg::Status::IO_ERROR);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, TooSmallBufferIsRefusedWithoutAnyRequest)
{
    uint8_t small[31];
    msg::FileHash tooSmall{"/a", etl::span<uint8_t>(small)};

    fileHash.handleRequest(tooSmall);

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::FileHash::BUFFER_TOO_SMALL, tooSmall.getStatus());
}

TEST(FileHash, BufferOfExactlyDigestSizeIsEnough)
{
    fileHash.handleRequest(request);

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
}

TEST(FileHash, BusyWhenThereIsAnotherRequest)
{
    fileHash.handleRequest(request);
    uint8_t otherOutput[32];
    msg::FileHash other{"/b", etl::span<uint8_t>(otherOutput)};

    fileHash.handleRequest(other);

    CHECK_EQUAL(msg::FileHash::BUSY, other.getStatus());
    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(1, api.responded.size());
}

TEST(FileHash, BusyIsReportedBeforeTheBufferIsChecked)
{
    fileHash.handleRequest(request);
    uint8_t small[1];
    msg::FileHash other{"/b", etl::span<uint8_t>(small)};

    fileHash.handleRequest(other);

    CHECK_EQUAL(msg::FileHash::BUSY, other.getStatus());
}

TEST(FileHash, OpensTheHashDeviceByName)
{
    startOpeningDevice();

    CHECK_EQUAL(device::msg::Type::OPEN, filesystem.nextType());
    auto message = filesystem.take();
    CHECK_TRUE(reinterpret_cast<device::msg::Open&>(message).getName() == "hash0");
}

TEST(FileHash, LimitsTheWaitForTheDeviceWithSingleShotTimer)
{
    startOpeningDevice();
    filesystem.take();

    CHECK_EQUAL(device::msg::Type::TIMER, filesystem.nextType());
    auto message = filesystem.take();
    auto& timer = reinterpret_cast<device::msg::Timer&>(message);
    CHECK_EQUAL(device::msg::Timer::SINGLE_SHOT, timer.getMode());
    CHECK_EQUAL(1000, timer.getInterval());
}

TEST(FileHash, NoAnswerFromTheDeviceIsHashNotFound)
{
    startOpeningDevice();

    fireTimer();

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::HASH_NOT_FOUND, request.getStatus());
}

TEST(FileHash, TimerAfterTheDeviceIsOpenedChangesNothing)
{
    startOpeningDevice();
    answerDeviceOpen(device::msg::Open::SUCCESS);

    fireTimer();

    CHECK_EQUAL(0, api.responded.size());
}

TEST(FileHash, TimerWithoutFreeSlotIsDeviceError)
{
    startOpeningDevice();

    fireTimer(device::msg::Timer::NO_MEMORY);

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, TimerWithoutRequestIsIgnored)
{
    fireTimer();

    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(0, api.sent.size());
}

TEST(FileHash, LockedDeviceIsHashBusy)
{
    startOpeningDevice();

    answerDeviceOpen(device::msg::Open::LOCKED);

    CHECK_EQUAL(msg::FileHash::HASH_BUSY, request.getStatus());
}

TEST(FileHash, DeviceOpenFailureIsDeviceError)
{
    startOpeningDevice();

    answerDeviceOpen(device::msg::Open::INIT_FAILURE);

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, OpensTheFileForReadingWhenTheDeviceIsOpened)
{
    startOpeningDevice();

    answerDeviceOpen(device::msg::Open::SUCCESS);
    dropTimer();

    CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
    STRCMP_EQUAL("/a", filesystem.nextPath());
    CHECK_EQUAL(fs::Flags::READ_ONLY, api.sent.front().input.open.flags);
}

TEST(FileHash, FileOpenFailureIsDeviceErrorAfterClosingTheDevice)
{
    startOpeningDevice();
    answerDeviceOpen(device::msg::Open::SUCCESS);
    dropTimer();

    filesystem.answerOpen(fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(device::msg::Type::CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == HASH);
    CHECK_EQUAL(0, api.responded.size());
    answerDeviceClose();
    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, ReadsTheFileInChunksAndEndsWithAccumulateEnd)
{
    startReading(6);

    CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == FILE);
    CHECK_EQUAL(4, api.sent.front().input.readWrite.length);
    filesystem.answerRead("abcd");

    CHECK_EQUAL(device::msg::Type::HASH, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == HASH);
    CHECK_EQUAL(device::msg::Hash::ACCUMULATE, api.sent.front().input.readWrite.mode);
    CHECK_EQUAL(4, api.sent.front().input.size);
    MEMCMP_EQUAL("abcd", api.sent.front().input.data, 4);
    answerHash();

    CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
    CHECK_EQUAL(2, api.sent.front().input.readWrite.length);
    filesystem.answerRead("ef");

    CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
    CHECK_EQUAL(2, api.sent.front().input.size);
    MEMCMP_EQUAL("ef", api.sent.front().input.data, 2);
}

TEST(FileHash, FileFillingTheChunkExactlyIsEndedByThatChunk)
{
    startReading(4);

    filesystem.answerRead("abcd");

    CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
}

TEST(FileHash, EmptyFileIsHashedWithoutData)
{
    startReading(0);

    filesystem.answerRead("");

    CHECK_EQUAL(device::msg::Type::HASH, filesystem.nextType());
    CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
    CHECK_EQUAL(0, api.sent.front().input.size);
}

TEST(FileHash, DigestIsWrittenToTheBufferOfTheClient)
{
    startReading(2);
    filesystem.answerRead("ab");

    POINTERS_EQUAL(output, api.sent.front().output.data);
    CHECK_EQUAL(32, api.sent.front().output.size);
    answerHash();

    BYTES_EQUAL(0xAB, output[0]);
    BYTES_EQUAL(0xAB, output[31]);
}

TEST(FileHash, ClosesTheFileAndTheDeviceWhenHashed)
{
    hashFile("ab");

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == FILE);
    api.sent.erase(api.sent.begin());
    CHECK_EQUAL(device::msg::Type::CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == HASH);
    CHECK_EQUAL(0, api.responded.size());
}

TEST(FileHash, AnswersSuccessAfterTheFileAndTheDeviceAreClosed)
{
    hashFile("ab");

    filesystem.answerClose();
    CHECK_EQUAL(0, api.responded.size());
    answerDeviceClose();

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::SUCCESS, request.getStatus());
}

TEST(FileHash, ClosingInTheOtherOrderIsTheSame)
{
    hashFile("ab");
    auto fileClose = filesystem.take();

    answerDeviceClose();
    CHECK_EQUAL(0, api.responded.size());
    reinterpret_cast<fs::msg::file::Close&>(fileClose).setStatus(fs::msg::Status::SUCCESS);
    fileHash.handleResponse(fileClose);

    CHECK_EQUAL(msg::FileHash::SUCCESS, request.getStatus());
}

// A failure answers the request only after the file and the device are closed, so they are not blocked.
TEST(FileHash, FailureIsAnsweredAfterTheFileAndTheDeviceAreClosed)
{
    startReading(5);

    filesystem.answerRead("", fs::msg::Status::IO_ERROR);

    CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == FILE);
    filesystem.answerClose();
    CHECK_EQUAL(0, api.responded.size());
    CHECK_EQUAL(device::msg::Type::CLOSE, filesystem.nextType());
    CHECK_TRUE(filesystem.nextRecipient() == HASH);
    answerDeviceClose();

    CHECK_FALSE(filesystem.hasRequest());
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, HashFailureIsDeviceErrorAfterClosing)
{
    startReading(5);
    filesystem.answerRead("abcd");

    answerHash(device::msg::Hash::DEVICE_FAILURE);
    filesystem.answerClose();
    answerDeviceClose();

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, FailureOfTheLastChunkIsDeviceErrorAfterClosing)
{
    startReading(2);
    filesystem.answerRead("ab");

    answerHash(device::msg::Hash::BUSY);
    filesystem.answerClose();
    answerDeviceClose();

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, FileCloseFailureIsDeviceError)
{
    hashFile("ab");

    filesystem.answerClose(fs::msg::Status::IO_ERROR);
    answerDeviceClose();

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, FailedCloseDoesNotHideTheFirstError)
{
    startReading(5);
    filesystem.answerRead("", fs::msg::Status::IO_ERROR);

    filesystem.answerClose(fs::msg::Status::IO_ERROR);
    answerDeviceClose(device::msg::Close::LOCKED);

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, ShortReadIsHashedWithWhatWasRead)
{
    startReading(10);
    filesystem.answerRead("abcd");
    answerHash();

    filesystem.answerRead("ef");

    CHECK_EQUAL(device::msg::Hash::ACCUMULATE, api.sent.front().input.readWrite.mode);
    CHECK_EQUAL(2, api.sent.front().input.size);
    answerHash();
    CHECK_EQUAL(4, api.sent.front().input.readWrite.length);
}

TEST(FileHash, FileEndingBeforeTheSizeEndsTheHashing)
{
    startReading(10);

    filesystem.answerRead("");

    CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
    CHECK_EQUAL(0, api.sent.front().input.size);
}

TEST(FileHash, DigestIsWrittenToTheFirstBytesOfALargerBuffer)
{
    uint8_t large[40] = {};
    msg::FileHash larger{"/a", etl::span<uint8_t>(large)};
    fileHash.handleRequest(larger);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    answerDeviceOpen(device::msg::Open::SUCCESS);
    dropTimer();
    filesystem.answerOpen(fs::msg::Status::SUCCESS, 2, FILE);
    filesystem.answerRead("ab");

    POINTERS_EQUAL(large, api.sent.front().output.data);
    CHECK_EQUAL(32, api.sent.front().output.size);
}

TEST(FileHash, DeviceCloseFailureIsDeviceError)
{
    hashFile("ab");
    filesystem.answerClose();

    answerDeviceClose(device::msg::Close::DEVICE_ERROR);

    CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
}

TEST(FileHash, TakesNextRequestWhenDone)
{
    hashFile("ab");
    filesystem.answerClose();
    answerDeviceClose();
    msg::FileHash other{"/c", etl::span<uint8_t>(output)};

    fileHash.handleRequest(other);

    CHECK_EQUAL(fs::msg::Type::STAT, filesystem.nextType());
    STRCMP_EQUAL("/c", filesystem.nextPath());
}

TEST(FileHash, RequestAfterFailureStartsClean)
{
    startReading(5);
    filesystem.answerRead("", fs::msg::Status::IO_ERROR);
    filesystem.answerClose();
    answerDeviceClose();

    fireTimer();

    startOpeningDevice();
    fireTimer();

    CHECK_EQUAL(msg::FileHash::HASH_NOT_FOUND, request.getStatus());
}

TEST(FileHash, TimerOfAnEarlierRequestDoesNotEndTheNewOne)
{
    startOpeningDevice();
    answerDeviceOpen(device::msg::Open::LOCKED);
    api.sent.clear();
    api.responded.clear();

    startOpeningDevice();
    fireTimer();
    CHECK_EQUAL(0, api.responded.size());

    fireTimer();
    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::HASH_NOT_FOUND, request.getStatus());
}

TEST(FileHash, LateAnswerOfTheDeviceIsIgnored)
{
    startOpeningDevice();
    auto lateOpen = api.sent.front();
    fireTimer();
    api.sent.clear();
    api.responded.clear();

    reinterpret_cast<device::msg::Open&>(lateOpen).setStatus(device::msg::Open::SUCCESS).setOpened(HASH);
    fileHash.handleResponse(lateOpen);

    CHECK_EQUAL(0, api.sent.size());
    CHECK_EQUAL(0, api.responded.size());
}

TEST(FileHash, SecondRequestWorksLikeTheFirst)
{
    hashFile("ab");
    filesystem.answerClose();
    answerDeviceClose();
    std::memset(output, 0, sizeof(output));
    api.responded.clear();

    fileHash.handleRequest(request);
    filesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    answerDeviceOpen(device::msg::Open::SUCCESS);
    dropTimer();
    filesystem.answerOpen(fs::msg::Status::SUCCESS, 2, FILE);
    filesystem.answerRead("ab");
    answerHash();
    filesystem.answerClose();
    answerDeviceClose();

    CHECK_EQUAL(1, api.responded.size());
    CHECK_EQUAL(msg::FileHash::SUCCESS, request.getStatus());
    BYTES_EQUAL(0xAB, output[0]);
}

TEST(FileHash, StaticVersionUsesItsOwnBuffer)
{
    service::FileHashStatic<8> staticHash{api, "hash0"};
    mock::Filesystem staticFilesystem{staticHash, api};

    staticHash.handleRequest(request);
    staticFilesystem.answerStat(fs::msg::Status::SUCCESS, fs::Info::FILE);
    auto opened = staticFilesystem.take();
    reinterpret_cast<device::msg::Open&>(opened).setStatus(device::msg::Open::SUCCESS).setOpened(HASH);
    staticHash.handleResponse(opened);
    dropTimer();
    auto message = staticFilesystem.take();
    reinterpret_cast<fs::msg::file::Open&>(message).setStatus(fs::msg::Status::SUCCESS).setOpened(FILE).setSize(20);
    staticHash.handleResponse(message);

    CHECK_EQUAL(8, api.sent.front().input.readWrite.length);
    CHECK_EQUAL(8, api.sent.front().output.size);
}

TEST(FileHash, IgnoresResponsesWithoutRequest)
{
    npos::ipc::Message message;

    for(auto type : std::initializer_list<npos::ipc::Message::Type>{ fs::msg::Type::STAT, fs::msg::Type::FILE_OPEN, fs::msg::Type::FILE_READ, fs::msg::Type::FILE_CLOSE,
        device::msg::Type::OPEN, device::msg::Type::HASH, device::msg::Type::CLOSE })
    {
        message.type = type;
        fileHash.handleResponse(message);
    }

    CHECK_EQUAL(0, api.sent.size());
    CHECK_EQUAL(0, api.responded.size());
}

TEST(FileHash, IgnoresUnexpectedResponses)
{
    fileHash.handleRequest(request);
    npos::ipc::Message message;
    message.type = fs::msg::Type::DIR_OPEN;

    fileHash.handleResponse(message);

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(0, api.responded.size());
}
