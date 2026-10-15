/**
 * @file Api.test.cpp
 * @author Adrian Szczepanski
 * @date 13-10-2026
 */

#include "mock/RecordingApi.hpp"

#include <libnpos/fs/Api.hpp>

#include <CppUTest/TestHarness.h>

namespace
{
    class Sender : public npos::ipc::Service
    {
    public:
        using npos::ipc::Service::Service;
        void handleRequest(npos::ipc::Message&) override {}
        void handleResponse(npos::ipc::Message&) override {}
        bool acceptsRequest(npos::ipc::Message::Type) const override { return false; }
    };
}

TEST_GROUP(FsApi)
{
    mock::RecordingApi api;
    Sender sender{api};
    uint8_t buffer[8];
};

TEST(FsApi, FileOpenSendsOpenRequest)
{
    npos::fs::api::file::open(sender, "f.txt", npos::fs::Flags::READ_ONLY);

    CHECK_EQUAL(1, api.sent.size());
    CHECK_EQUAL(npos::fs::msg::Type::FILE_OPEN, api.sent[0].type);
}

TEST(FsApi, FileCloseSendsCloseRequestToFile)
{
    npos::fs::api::file::close(sender, { 1, 2 });

    CHECK_EQUAL(npos::fs::msg::Type::FILE_CLOSE, api.sent[0].type);
    CHECK_EQUAL(2, api.sent[0].recipient.serviceId);
}

TEST(FsApi, FileReadSendsReadRequest)
{
    npos::fs::api::file::read(sender, { 1, 2 }, 4, etl::span<uint8_t>(buffer, sizeof(buffer)));

    CHECK_EQUAL(npos::fs::msg::Type::FILE_READ, api.sent[0].type);
    CHECK_EQUAL(4, api.sent[0].input.readWrite.length);
}

TEST(FsApi, FileWriteSendsWriteRequest)
{
    npos::fs::api::file::write(sender, { 1, 2 }, etl::span<const uint8_t>(buffer, 3));

    CHECK_EQUAL(npos::fs::msg::Type::FILE_WRITE, api.sent[0].type);
    CHECK_EQUAL(3, api.sent[0].input.size);
}

TEST(FsApi, DirOpenSendsOpenRequest)
{
    npos::fs::api::dir::open(sender, "/");

    CHECK_EQUAL(npos::fs::msg::Type::DIR_OPEN, api.sent[0].type);
}

TEST(FsApi, DirCloseSendsCloseRequestToDirectory)
{
    npos::fs::api::dir::close(sender, { 1, 3 });

    CHECK_EQUAL(npos::fs::msg::Type::DIR_CLOSE, api.sent[0].type);
    CHECK_EQUAL(3, api.sent[0].recipient.serviceId);
}

TEST(FsApi, DirMakeSendsMakeRequest)
{
    npos::fs::api::dir::make(sender, "/new");

    CHECK_EQUAL(npos::fs::msg::Type::DIR_MAKE, api.sent[0].type);
}

TEST(FsApi, DirReadSendsReadRequestWithEntryBuffer)
{
    npos::fs::msg::dir::Entry entry;

    npos::fs::api::dir::read(sender, { 1, 3 }, entry);

    CHECK_EQUAL(npos::fs::msg::Type::DIR_READ, api.sent[0].type);
    POINTERS_EQUAL(&entry, api.sent[0].output.data);
}
