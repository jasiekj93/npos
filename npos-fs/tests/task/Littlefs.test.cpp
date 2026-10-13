/**
 * @file Littlefs.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/FakeBus.hpp"
#include "mock/FakeICache.hpp"
#include "mock/RamFlash.hpp"

#include <libnpos/fs/Api.hpp>
#include <libnpos/fs/task/Littlefs.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::fs;

TEST_GROUP(Littlefs)
{
    static constexpr size_t SLOTS = 2;

    mock::FakeBus bus;
    mock::RamFlash flash;
    mock::FakeICache icache;
    task::Littlefs<SLOTS>::MessageQueue<8> queue;
    task::Littlefs<SLOTS> filesystem{3, queue, bus, flash, &icache};

    void setup() override
    {
        filesystem.setId(2);
        filesystem.initialize();
    }

    void run(npos::ipc::Message& message)
    {
        filesystem.receive(message);
        filesystem.process();
    }

    npos::ipc::Oid open(const char* path, uint32_t flags)
    {
        msg::file::Open request(path, flags);
        run(request);
        CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
        return request.getOpened();
    }

    void close(npos::ipc::Oid file)
    {
        msg::file::Close request(file);
        run(request);
        CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    }

    void write(const char* path, const char* text, uint32_t flags = Flags::WRITE_ONLY | Flags::CREATE | Flags::TRUNCATE)
    {
        auto file = open(path, flags);
        msg::file::Write request(file, etl::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text), strlen(text)));
        run(request);
        CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
        CHECK_EQUAL(strlen(text), request.getOutputLength());
        close(file);
    }

    npos::ipc::Oid openDir(const char* path)
    {
        msg::dir::Open request(path);
        run(request);
        CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
        return request.getOpened();
    }
};

TEST(Littlefs, KeepsPriority)
{
    CHECK_EQUAL(3, filesystem.getPriority());
}

TEST(Littlefs, InitializeInitializesCacheAndFormatsBlankFlash)
{
    CHECK_EQUAL(1, icache.initCount);
    CHECK_TRUE(flash.programCount > 0);
}

TEST(Littlefs, InitializeWorksWithoutICache)
{
    mock::RamFlash otherFlash;
    task::Littlefs<SLOTS>::MessageQueue<2> otherQueue;
    task::Littlefs<SLOTS> other(3, otherQueue, bus, otherFlash, nullptr);

    other.initialize();

    CHECK_TRUE(otherFlash.programCount > 0);
}

TEST(Littlefs, InitializeSurvivesFlashThatCannotBeProgrammed)
{
    mock::RamFlash brokenFlash;
    brokenFlash.failUnlock = true;
    task::Littlefs<SLOTS>::MessageQueue<2> otherQueue;
    task::Littlefs<SLOTS> other(3, otherQueue, bus, brokenFlash, nullptr);

    other.initialize();

    CHECK_EQUAL(0, brokenFlash.programCount);
}

TEST(Littlefs, AcceptsFileAndDirectoryMessagesOnly)
{
    CHECK_TRUE(filesystem.accepts(msg::Type::FILE_OPEN));
    CHECK_TRUE(filesystem.accepts(msg::Type::FILE_CLOSE));
    CHECK_TRUE(filesystem.accepts(msg::Type::FILE_READ));
    CHECK_TRUE(filesystem.accepts(msg::Type::FILE_WRITE));
    CHECK_TRUE(filesystem.accepts(msg::Type::DIR_OPEN));
    CHECK_TRUE(filesystem.accepts(msg::Type::DIR_CLOSE));
    CHECK_TRUE(filesystem.accepts(msg::Type::DIR_READ));
    CHECK_FALSE(filesystem.accepts(npos::ipc::DEVICE_MESSAGE_TYPE));
}

TEST(Littlefs, IsReadyOnlyWithQueuedMessages)
{
    msg::file::Open request("a", Flags::READ_ONLY);

    CHECK_FALSE(filesystem.isReady());

    filesystem.receive(request);

    CHECK_TRUE(filesystem.isReady());

    filesystem.process();

    CHECK_FALSE(filesystem.isReady());
}

TEST(Littlefs, ReceiveDropsMessagesWhenQueueIsFull)
{
    mock::RamFlash smallFlash;
    task::Littlefs<SLOTS>::MessageQueue<1> smallQueue;
    task::Littlefs<SLOTS> small(3, smallQueue, bus, smallFlash, nullptr);
    small.initialize();
    msg::file::Open a("a", Flags::READ_ONLY), b("b", Flags::READ_ONLY);

    small.receive(a);
    small.receive(b);
    small.process();

    CHECK_FALSE(small.isReady());
}

TEST(Littlefs, ProcessWithoutMessagesDoesNothing)
{
    filesystem.process();

    CHECK_EQUAL(0, bus.responded.size());
}

TEST(Littlefs, ProcessReleasesTheMessage)
{
    msg::file::Open request("a", Flags::READ_ONLY);

    run(request);

    CHECK_EQUAL(1, bus.released.size());
}

TEST(Littlefs, IgnoresUnknownMessageTypes)
{
    npos::ipc::Message message;
    message.type = npos::ipc::DEVICE_MESSAGE_TYPE;

    run(message);

    CHECK_EQUAL(0, bus.responded.size());
}

TEST(Littlefs, OpeningMissingFileFails)
{
    msg::file::Open request("missing.txt", Flags::READ_ONLY);

    run(request);

    CHECK_EQUAL(msg::Status::NO_DIR_ENTRY, request.getStatus());
    CHECK_EQUAL(1, bus.responded.size());
}

TEST(Littlefs, OpenedFileOidPointsAtThisPortAndSlot)
{
    auto file = open("a.txt", Flags::WRITE_ONLY | Flags::CREATE);

    CHECK_EQUAL(2, file.portId);
    CHECK_EQUAL(0, file.serviceId);
}

TEST(Littlefs, OpenFailsWithNoMemoryWhenAllSlotsAreUsed)
{
    open("a", Flags::WRITE_ONLY | Flags::CREATE);
    open("b", Flags::WRITE_ONLY | Flags::CREATE);
    msg::file::Open request("c", Flags::WRITE_ONLY | Flags::CREATE);

    run(request);

    CHECK_EQUAL(msg::Status::NO_MEMORY, request.getStatus());
}

TEST(Littlefs, ClosedSlotCanBeReused)
{
    auto first = open("a", Flags::WRITE_ONLY | Flags::CREATE);
    open("b", Flags::WRITE_ONLY | Flags::CREATE);
    close(first);

    auto again = open("c", Flags::WRITE_ONLY | Flags::CREATE);

    CHECK_EQUAL(first.serviceId, again.serviceId);
}

TEST(Littlefs, WrittenDataCanBeReadBack)
{
    write("hello.txt", "Hello, World!");
    auto file = open("hello.txt", Flags::READ_ONLY);
    uint8_t buffer[32] = {};

    msg::file::Read request(file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));
    run(request);

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
    CHECK_EQUAL(13, request.getOutputLength());
    STRCMP_EQUAL("Hello, World!", reinterpret_cast<const char*>(buffer));
    close(file);
}

TEST(Littlefs, ReadAtEndOfFileReturnsZeroLength)
{
    write("a", "xy");
    auto file = open("a", Flags::READ_ONLY);
    uint8_t buffer[8];
    msg::file::Read first(file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));
    run(first);

    msg::file::Read second(file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));
    run(second);

    CHECK_EQUAL(2, first.getOutputLength());
    CHECK_EQUAL(0, second.getOutputLength());
    CHECK_EQUAL(msg::Status::SUCCESS, second.getStatus());
}

TEST(Littlefs, AppendKeepsExistingContent)
{
    write("a", "abc");
    write("a", "def", Flags::WRITE_ONLY | Flags::APPEND);
    auto file = open("a", Flags::READ_ONLY);
    uint8_t buffer[16] = {};
    msg::file::Read request(file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));

    run(request);

    CHECK_EQUAL(6, request.getOutputLength());
    STRCMP_EQUAL("abcdef", reinterpret_cast<const char*>(buffer));
}

TEST(Littlefs, TruncateDiscardsExistingContent)
{
    write("a", "abcdef");
    write("a", "x");
    auto file = open("a", Flags::READ_ONLY);
    uint8_t buffer[16] = {};
    msg::file::Read request(file, sizeof(buffer), etl::span<uint8_t>(buffer, sizeof(buffer)));

    run(request);

    CHECK_EQUAL(1, request.getOutputLength());
}

TEST(Littlefs, ExclusiveCreateOfExistingFileFails)
{
    write("a", "x");
    msg::file::Open request("a", Flags::WRITE_ONLY | Flags::CREATE | Flags::EXCLUSIVE);

    run(request);

    CHECK_EQUAL(msg::Status::ENTRY_EXISTS, request.getStatus());
}

TEST(Littlefs, CloseOfUnknownFileFails)
{
    msg::file::Close request({ 2, 0 });

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, CloseOfOutOfRangeFileFails)
{
    msg::file::Close request({ 2, 99 });

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, ReadOfUnknownFileFails)
{
    uint8_t buffer[4];
    msg::file::Read request({ 2, 0 }, 4, etl::span<uint8_t>(buffer, sizeof(buffer)));

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, WriteToUnknownFileFails)
{
    const uint8_t data[2] = { 1, 2 };
    msg::file::Write request({ 2, 0 }, etl::span<const uint8_t>(data, sizeof(data)));

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, WriteReportsFlashFailure)
{
    auto file = open("a", Flags::WRITE_ONLY | Flags::CREATE);
    flash.failProgram = true;
    uint8_t data[512];
    memset(data, 'x', sizeof(data));
    msg::file::Write write(file, etl::span<const uint8_t>(data, sizeof(data)));
    run(write);
    msg::file::Close close(file);

    run(close);

    CHECK_TRUE(close.getStatus() != msg::Status::SUCCESS or write.getStatus() != msg::Status::SUCCESS);
}

TEST(Littlefs, ProgrammingDisablesAndReenablesICache)
{
    auto disabledBefore = icache.disableCount;

    write("a", "x");

    CHECK_TRUE(icache.disableCount > disabledBefore);
    CHECK_EQUAL(icache.disableCount, icache.enableCount - 0);
}

TEST(Littlefs, OpenInDirectoryThatDoesNotExistFails)
{
    msg::file::Open request("nodir/a", Flags::WRITE_ONLY | Flags::CREATE);

    run(request);

    CHECK_EQUAL(msg::Status::NO_DIR_ENTRY, request.getStatus());
}

TEST(Littlefs, OpeningDirectoryAsFileFails)
{
    msg::file::Open request("/", Flags::READ_ONLY);

    run(request);

    CHECK_EQUAL(msg::Status::IS_A_DIRECTORY, request.getStatus());
}

TEST(Littlefs, OpeningRootDirectorySucceeds)
{
    auto dir = openDir("/");

    CHECK_EQUAL(2, dir.portId);
    CHECK_EQUAL(0, dir.serviceId);
}

TEST(Littlefs, OpeningMissingDirectoryFails)
{
    msg::dir::Open request("/nothing");

    run(request);

    CHECK_EQUAL(msg::Status::NO_DIR_ENTRY, request.getStatus());
}

TEST(Littlefs, OpeningFileAsDirectoryFails)
{
    write("a", "x");
    msg::dir::Open request("a");

    run(request);

    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, request.getStatus());
}

TEST(Littlefs, DirectoryOpenFailsWithNoMemoryWhenAllSlotsAreUsed)
{
    openDir("/");
    openDir("/");
    msg::dir::Open request("/");

    run(request);

    CHECK_EQUAL(msg::Status::NO_MEMORY, request.getStatus());
}

TEST(Littlefs, EmptyDirectoryEndsImmediately)
{
    auto dir = openDir("/");
    msg::dir::Entry entry;
    msg::dir::Read request(dir, entry);

    run(request);

    CHECK_EQUAL(msg::Status::END_OF_DIRECTORY, request.getStatus());
}

TEST(Littlefs, DirectoryListsFilesWithSizeAndSkipsDotEntries)
{
    write("a.txt", "12345");
    write("b.txt", "1");
    auto dir = openDir("/");
    msg::dir::Entry entry;
    int count = 0;
    bool foundA = false;

    for(;;)
    {
        msg::dir::Read request(dir, entry);
        run(request);

        if(request.getStatus() == msg::Status::END_OF_DIRECTORY)
            break;

        CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
        CHECK_TRUE(entry.getName() != "." and entry.getName() != "..");
        CHECK_EQUAL(msg::dir::Entry::FILE, entry.type);

        if(entry.getName() == "a.txt")
        {
            foundA = true;
            CHECK_EQUAL(5, entry.size);
        }

        count++;
    }

    CHECK_EQUAL(2, count);
    CHECK_TRUE(foundA);
}

TEST(Littlefs, DirectoryCloseFreesTheSlot)
{
    auto first = openDir("/");
    openDir("/");
    msg::dir::Close close(first);
    run(close);

    CHECK_EQUAL(msg::Status::SUCCESS, close.getStatus());

    auto again = openDir("/");

    CHECK_EQUAL(first.serviceId, again.serviceId);
}

TEST(Littlefs, DirectoryCloseOfUnknownDirectoryFails)
{
    msg::dir::Close request({ 2, 0 });

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, DirectoryReadOfUnknownDirectoryFails)
{
    msg::dir::Entry entry;
    msg::dir::Read request({ 2, 7 }, entry);

    run(request);

    CHECK_EQUAL(msg::Status::BAD_FILE_NUMBER, request.getStatus());
}

TEST(Littlefs, FileAndDirectoryHandlesAreIndependent)
{
    auto file = open("a", Flags::WRITE_ONLY | Flags::CREATE);
    auto dir = openDir("/");

    CHECK_EQUAL(file.serviceId, dir.serviceId);

    msg::dir::Close closeDir(dir);
    run(closeDir);

    CHECK_EQUAL(msg::Status::SUCCESS, closeDir.getStatus());
    close(file);
}

TEST(Littlefs, DataSurvivesRemount)
{
    write("keep.txt", "persistent");

    task::Littlefs<SLOTS>::MessageQueue<4> otherQueue;
    task::Littlefs<SLOTS> remounted(3, otherQueue, bus, flash, nullptr);
    remounted.setId(2);
    remounted.initialize();
    msg::file::Open request("keep.txt", Flags::READ_ONLY);
    remounted.receive(request);
    remounted.process();

    CHECK_EQUAL(msg::Status::SUCCESS, request.getStatus());
}
