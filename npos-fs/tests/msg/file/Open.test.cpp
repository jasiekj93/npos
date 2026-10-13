/**
 * @file Open.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include <libnpos/fs/msg/file/Open.hpp>

#include <CppUTest/TestHarness.h>

TEST_GROUP(FileOpen)
{
};

TEST(FileOpen, StoresPathAndFlags)
{
    npos::fs::msg::file::Open request("a/b.txt", npos::fs::Flags::READ_ONLY | npos::fs::Flags::CREATE);

    CHECK_EQUAL(npos::fs::msg::Type::FILE_OPEN, request.type);
    STRCMP_EQUAL("a/b.txt", request.getPath().data());
    CHECK_EQUAL(7, request.getPath().size());
    CHECK_EQUAL(npos::fs::Flags::READ_ONLY | npos::fs::Flags::CREATE, request.getFlags());
}

TEST(FileOpen, IsBroadcastUntilAnswered)
{
    npos::fs::msg::file::Open request("x", 0);

    CHECK_EQUAL(npos::ipc::NULL_PORT, request.recipient.portId);
}

TEST(FileOpen, ResponseCarriesStatusAndOpenedOid)
{
    npos::fs::msg::file::Open request("x", 0);

    request.setStatus(npos::fs::msg::Status::NO_DIR_ENTRY).setOpened({ 2, 5 });

    CHECK_EQUAL(npos::fs::msg::Status::NO_DIR_ENTRY, request.getStatus());
    CHECK_EQUAL(2, request.getOpened().portId);
    CHECK_EQUAL(5, request.getOpened().serviceId);
}
