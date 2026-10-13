/**
 * @file DirRead.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/dir/Read.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(DirRead)
{
    npos::fs::msg::dir::Entry entry;
};

TEST(DirRead, EntryNameIsBoundedByNameSize)
{
    for(auto& c : entry.name)
        c = 'x';

    CHECK_EQUAL(npos::fs::msg::dir::Entry::NAME_SIZE, entry.getName().size());
}

TEST(DirRead, EntryNameStopsAtTerminator)
{
    entry.name[0] = 'a';
    entry.name[1] = 'b';
    entry.name[2] = 0;

    CHECK_EQUAL(2, entry.getName().size());
    CHECK_TRUE(entry.getName() == "ab");
}

TEST(DirRead, EntryDefaultsToEmptyFile)
{
    CHECK_EQUAL(npos::fs::msg::dir::Entry::FILE, entry.type);
    CHECK_EQUAL(0, entry.size);
    CHECK_EQUAL(0, entry.getName().size());
}

TEST(DirRead, ReadTargetsDirectoryAndEntry)
{
    npos::fs::msg::dir::Read request({ 2, 3 }, entry);

    CHECK_EQUAL(npos::fs::msg::Type::DIR_READ, request.type);
    CHECK_EQUAL(3, request.getDir().serviceId);
    POINTERS_EQUAL(&entry, &request.getEntry());
}

TEST(DirRead, ResponseCarriesStatus)
{
    npos::fs::msg::dir::Read request({ 2, 3 }, entry);

    request.setStatus(npos::fs::msg::Status::END_OF_DIRECTORY);

    CHECK_EQUAL(npos::fs::msg::Status::END_OF_DIRECTORY, request.getStatus());
}
