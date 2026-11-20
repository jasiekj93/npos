// /**
//  * @file FileHash.test.cpp
//  * @author Adrian Szczepanski
//  * @date 19-11-2026
//  */

// #include "mock/Filesystem.hpp"

// #include <libnpos/mdlwr/dev/service/FileHash.hpp>

// #include <CppUTest/TestHarness.h>

// using namespace npos::mdlwr::dev;
// namespace fs = npos::fs;
// namespace device = npos::dev;

// TEST_GROUP(FileHash)
// {
//     static constexpr npos::ipc::Oid HASH{ 6, 2 };

//     mock::RecordingApi api;
//     uint8_t chunk[4];
//     service::FileHash fileHash{api, "hash0", etl::span<uint8_t>(chunk)};
//     mock::Filesystem filesystem{fileHash, api};
//     uint8_t output[40] = {};
//     msg::FileHash request{"/a", etl::span<uint8_t>(output)};

//     void setup() override
//     {
//         fileHash.initialize();
//     }

//     // The timer limiting the wait for the device is sent right after the open request, it stays unanswered.
//     void answerHashOpen(device::msg::Open::Status status, npos::ipc::Oid opened = HASH)
//     {
//         auto message = filesystem.take();
//         CHECK_EQUAL(device::msg::Type::TIMER, filesystem.nextType());
//         filesystem.take();
//         reinterpret_cast<device::msg::Open&>(message).setStatus(status).setOpened(opened);
//         fileHash.handleResponse(message);
//     }

//     void fireTimer(device::msg::Timer::Status status = device::msg::Timer::SUCCESS)
//     {
//         npos::ipc::Message message;
//         message.type = device::msg::Type::TIMER;
//         reinterpret_cast<device::msg::Timer&>(message).setStatus(status);
//         fileHash.handleResponse(message);
//     }

//     void answerHash(device::msg::Hash::Status status = device::msg::Hash::SUCCESS)
//     {
//         auto message = filesystem.take();
//         auto& response = reinterpret_cast<device::msg::Hash&>(message);

//         if(status == device::msg::Hash::SUCCESS and response.getOperation() == device::msg::Hash::ACCUMULATE_END)
//             std::memset(response.getOutput().data(), 0xAB, response.getOutput().size());

//         response.setStatus(status);
//         fileHash.handleResponse(message);
//     }

//     void answerHashClose(device::msg::Close::Status status = device::msg::Close::SUCCESS)
//     {
//         auto message = filesystem.take();
//         reinterpret_cast<device::msg::Close&>(message).setStatus(status);
//         fileHash.handleResponse(message);
//     }

//     // Takes the request through opening the file and the device, up to the first read.
//     void startReading(size_t fileSize)
//     {
//         fileHash.handleRequest(request);
//         filesystem.answerOpen(fs::msg::Status::SUCCESS, fileSize, { 5, 0 });
//         answerHashOpen(device::msg::Open::SUCCESS);
//     }

//     // The closing of the file and of the device that ends every operation.
//     void answerClosing()
//     {
//         CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
//         filesystem.answerClose();
//         CHECK_EQUAL(device::msg::Type::CLOSE, filesystem.nextType());
//         CHECK_TRUE(filesystem.nextRecipient() == HASH);
//         answerHashClose();
//     }
// };

// TEST(FileHash, AcceptsOnlyItsRequest)
// {
//     CHECK_TRUE(fileHash.acceptsRequest(msg::Type::FILE_HASH));
//     CHECK_FALSE(fileHash.acceptsRequest(msg::Type::FILE_HASH + 1));
// }

// TEST(FileHash, OpensTheFileForReading)
// {
//     fileHash.handleRequest(request);

//     CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
//     STRCMP_EQUAL("/a", filesystem.nextPath());
//     CHECK_EQUAL(fs::Flags::READ_ONLY, api.sent.front().input.open.flags);
// }

// TEST(FileHash, OpensTheHashDeviceByNameWhenTheFileIsOpened)
// {
//     fileHash.handleRequest(request);

//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     CHECK_EQUAL(device::msg::Type::OPEN, filesystem.nextType());
//     auto message = filesystem.take();
//     CHECK_TRUE(reinterpret_cast<device::msg::Open&>(message).getName() == "hash0");
// }

// TEST(FileHash, LimitsTheWaitForTheDeviceWithSingleShotTimer)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);
//     filesystem.take();

//     CHECK_EQUAL(device::msg::Type::TIMER, filesystem.nextType());
//     auto message = filesystem.take();
//     auto& timer = reinterpret_cast<device::msg::Timer&>(message);
//     CHECK_EQUAL(device::msg::Timer::SINGLE_SHOT, timer.getMode());
//     CHECK_EQUAL(1000, timer.getInterval());
// }

// TEST(FileHash, NoAnswerFromTheDeviceIsHashNotFoundAfterClosingTheFile)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     fireTimer();

//     CHECK_EQUAL(0, api.responded.size());
//     api.sent.erase(api.sent.begin(), api.sent.end() - 1);
//     CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
//     filesystem.answerClose();
//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::HASH_NOT_FOUND, request.getStatus());
// }

// TEST(FileHash, TimerAfterTheDeviceIsOpenedChangesNothing)
// {
//     startReading(5);

//     fireTimer();

//     CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
//     CHECK_EQUAL(0, api.responded.size());
// }

// TEST(FileHash, TimerWithoutFreeSlotDoesNotEndTheWait)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     fireTimer(device::msg::Timer::NO_MEMORY);
//     answerHashOpen(device::msg::Open::SUCCESS);

//     CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
// }

// TEST(FileHash, TimerOfAnEarlierRequestDoesNotEndTheNewOne)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);
//     answerHashOpen(device::msg::Open::LOCKED);
//     filesystem.answerClose();
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     fireTimer();

//     CHECK_EQUAL(1, api.responded.size());
//     answerHashOpen(device::msg::Open::SUCCESS);
//     CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
// }

// TEST(FileHash, ReadsTheFileInChunksAndEndsWithAccumulateEnd)
// {
//     startReading(6);

//     CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
//     CHECK_TRUE(filesystem.nextRecipient() == (npos::ipc::Oid{ 5, 0 }));
//     CHECK_EQUAL(4, api.sent.front().output.size);
//     filesystem.answerRead("abcd");

//     CHECK_EQUAL(device::msg::Type::HASH, filesystem.nextType());
//     CHECK_TRUE(filesystem.nextRecipient() == HASH);
//     CHECK_EQUAL(device::msg::Hash::ACCUMULATE, api.sent.front().input.readWrite.mode);
//     CHECK_EQUAL(4, api.sent.front().input.size);
//     MEMCMP_EQUAL("abcd", api.sent.front().input.data, 4);
//     answerHash();

//     CHECK_EQUAL(fs::msg::Type::FILE_READ, filesystem.nextType());
//     CHECK_EQUAL(2, api.sent.front().output.size);
//     filesystem.answerRead("ef");

//     CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
//     CHECK_EQUAL(2, api.sent.front().input.size);
//     MEMCMP_EQUAL("ef", api.sent.front().input.data, 2);
// }

// TEST(FileHash, DigestGoesToTheBufferOfTheClient)
// {
//     startReading(2);
//     filesystem.answerRead("ab");
//     POINTERS_EQUAL(output, api.sent.front().output.data);
//     CHECK_EQUAL(npos::mdlwr::dev::service::FileHash::HASH_SIZE, api.sent.front().output.size);
//     answerHash();

//     answerClosing();

//     CHECK_EQUAL(1, api.responded.size());
//     CHECK_EQUAL(msg::FileHash::SUCCESS, request.getStatus());
//     BYTES_EQUAL(0xAB, output[0]);
//     BYTES_EQUAL(0xAB, output[31]);
//     BYTES_EQUAL(0, output[32]);
// }

// TEST(FileHash, AnswersOnlyAfterTheFileAndTheDeviceAreClosed)
// {
//     startReading(2);
//     filesystem.answerRead("ab");
//     answerHash();

//     CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
//     filesystem.answerClose();
//     CHECK_EQUAL(0, api.responded.size());
//     CHECK_EQUAL(device::msg::Type::CLOSE, filesystem.nextType());
//     answerHashClose();
//     CHECK_EQUAL(1, api.responded.size());
// }

// TEST(FileHash, FileFillingChunkExactlyIsEndedByThatChunk)
// {
//     startReading(4);

//     filesystem.answerRead("abcd");

//     CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
// }

// TEST(FileHash, EmptyFileIsHashedWithoutReading)
// {
//     startReading(0);

//     CHECK_EQUAL(device::msg::Type::HASH, filesystem.nextType());
//     CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
//     CHECK_EQUAL(0, api.sent.front().input.size);
//     answerHash();
//     answerClosing();

//     CHECK_EQUAL(msg::FileHash::SUCCESS, request.getStatus());
// }

// TEST(FileHash, ShrunkFileEndsWithWhatWasRead)
// {
//     startReading(10);
//     filesystem.answerRead("abcd");
//     answerHash();

//     filesystem.answerRead("");

//     CHECK_EQUAL(device::msg::Hash::ACCUMULATE_END, api.sent.front().input.readWrite.mode);
//     CHECK_EQUAL(0, api.sent.front().input.size);
// }

// TEST(FileHash, BusyWhenThereIsAnotherRequest)
// {
//     fileHash.handleRequest(request);
//     uint8_t otherOutput[32];
//     msg::FileHash other{"/b", etl::span<uint8_t>(otherOutput)};

//     fileHash.handleRequest(other);

//     CHECK_EQUAL(msg::FileHash::BUSY, other.getStatus());
//     CHECK_EQUAL(1, api.sent.size());
//     CHECK_EQUAL(1, api.responded.size());
// }

// TEST(FileHash, TooSmallBufferIsRefusedWithoutOpeningAnything)
// {
//     uint8_t small[31];
//     msg::FileHash tooSmall{"/a", etl::span<uint8_t>(small)};

//     fileHash.handleRequest(tooSmall);

//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::BUFFER_TOO_SMALL, tooSmall.getStatus());
// }

// TEST(FileHash, BufferOfExactlyDigestSizeIsEnough)
// {
//     uint8_t exact[32];
//     msg::FileHash fits{"/a", etl::span<uint8_t>(exact)};

//     fileHash.handleRequest(fits);

//     CHECK_EQUAL(fs::msg::Type::FILE_OPEN, filesystem.nextType());
// }

// TEST(FileHash, MissingFileIsReported)
// {
//     fileHash.handleRequest(request);

//     filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);

//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::FILE_NOT_FOUND, request.getStatus());
// }

// TEST(FileHash, OtherFileOpenErrorIsDeviceFailure)
// {
//     fileHash.handleRequest(request);

//     filesystem.answerOpen(fs::msg::Status::IO_ERROR);

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, TooLongPathIsReportedAsInvalid)
// {
//     etl::string<100> longPath(70, 'a');
//     msg::FileHash tooLong{etl::string_view(longPath.c_str(), longPath.size()), etl::span<uint8_t>(output)};

//     fileHash.handleRequest(tooLong);

//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::INVALID_PATH, tooLong.getStatus());
// }

// TEST(FileHash, DirectoryIsReported)
// {
//     fileHash.handleRequest(request);

//     filesystem.answerOpen(fs::msg::Status::IS_A_DIRECTORY);

//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::IS_A_DIRECTORY, request.getStatus());
// }

// TEST(FileHash, RejectedNameByTheFilesystemIsInvalidPath)
// {
//     fileHash.handleRequest(request);

//     filesystem.answerOpen(fs::msg::Status::NAME_TOO_LONG);

//     CHECK_EQUAL(msg::FileHash::INVALID_PATH, request.getStatus());
// }

// TEST(FileHash, DeviceReportingNotFoundIsHashNotFound)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     answerHashOpen(device::msg::Open::NOT_FOUND);
//     filesystem.answerClose();

//     CHECK_EQUAL(msg::FileHash::HASH_NOT_FOUND, request.getStatus());
// }

// TEST(FileHash, LockedDeviceIsReportedAfterClosingTheFile)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     answerHashOpen(device::msg::Open::LOCKED);

//     CHECK_EQUAL(fs::msg::Type::FILE_CLOSE, filesystem.nextType());
//     CHECK_EQUAL(0, api.responded.size());
//     filesystem.answerClose();
//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::HASH_BUSY, request.getStatus());
// }

// TEST(FileHash, DeviceOpenFailureIsDeviceFailure)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);

//     answerHashOpen(device::msg::Open::INIT_FAILURE);
//     filesystem.answerClose();

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, ReadFailureIsDeviceFailureAfterClosingBoth)
// {
//     startReading(5);

//     filesystem.answerRead("", fs::msg::Status::IO_ERROR);
//     answerClosing();

//     CHECK_FALSE(filesystem.hasRequest());
//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
//     BYTES_EQUAL(0, output[0]);
// }

// TEST(FileHash, HashFailureIsDeviceFailureAfterClosingBoth)
// {
//     startReading(5);
//     filesystem.answerRead("abcd");

//     answerHash(device::msg::Hash::DEVICE_FAILURE);
//     answerClosing();

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, FailureOfTheLastChunkIsDeviceFailure)
// {
//     startReading(2);
//     filesystem.answerRead("ab");

//     answerHash(device::msg::Hash::BUSY);
//     answerClosing();

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, FailedCloseTurnsSuccessIntoDeviceFailure)
// {
//     startReading(2);
//     filesystem.answerRead("ab");
//     answerHash();

//     filesystem.answerClose(fs::msg::Status::IO_ERROR);
//     answerHashClose();

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, FailedDeviceCloseTurnsSuccessIntoDeviceFailure)
// {
//     startReading(2);
//     filesystem.answerRead("ab");
//     answerHash();
//     filesystem.answerClose();

//     answerHashClose(device::msg::Close::DEVICE_ERROR);

//     CHECK_EQUAL(msg::FileHash::DEVICE_ERROR, request.getStatus());
// }

// TEST(FileHash, FailedCloseDoesNotHideTheFirstError)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::SUCCESS, 5);
//     answerHashOpen(device::msg::Open::LOCKED);

//     filesystem.answerClose(fs::msg::Status::IO_ERROR);

//     CHECK_EQUAL(msg::FileHash::HASH_BUSY, request.getStatus());
// }

// TEST(FileHash, TakesNextRequestWhenDone)
// {
//     fileHash.handleRequest(request);
//     filesystem.answerOpen(fs::msg::Status::NO_DIR_ENTRY);
//     msg::FileHash other{"/c", etl::span<uint8_t>(output)};

//     fileHash.handleRequest(other);

//     STRCMP_EQUAL("/c", filesystem.nextPath());
// }