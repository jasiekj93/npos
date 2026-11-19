/**
 * @file Middleware.test.cpp
 * @author Adrian Szczepanski
 * @date 18-11-2026
 */

#include "mock/FakeHost.hpp"
#include "mock/RamFlash.hpp"
#include "mock/FakeICache.hpp"

#include <etl/array.h>

#include <libnpos/ipc/ServiceTask.hpp>
#include <libnpos/ipc/SystemBus.hpp>
#include <libnpos/fs/task/Littlefs.hpp>
#include <libnpos/mdlwr/cli/program/Cp.hpp>
#include <libnpos/mdlwr/cli/program/Mv.hpp>
#include <libnpos/mdlwr/cli/program/Rm.hpp>
#include <libnpos/mdlwr/fs/service/dir/Copy.hpp>
#include <libnpos/mdlwr/fs/service/dir/Move.hpp>
#include <libnpos/mdlwr/fs/service/dir/Remove.hpp>
#include <libnpos/mdlwr/fs/service/file/Copy.hpp>
#include <libnpos/mdlwr/fs/service/file/Move.hpp>
#include <libnpos/mdlwr/fs/service/file/Read.hpp>
#include <libnpos/mdlwr/fs/service/file/Write.hpp>

#include <CppUTest/TestHarness.h>

using namespace npos::mdlwr::fs;
namespace fs = npos::fs;

namespace
{
    // Sends requests like an application would and remembers the last response.
    class Client : public npos::ipc::Service
    {
    public:
        using npos::ipc::Service::Service;

        void handleRequest(npos::ipc::Message&) override {}
        bool acceptsRequest(npos::ipc::Message::Type) const override { return false; }
        void handleResponse(npos::ipc::Message& message) override
        {
            response = message;
            answered = true;
        }

        template<typename T>
        T& last() { return reinterpret_cast<T&>(response); }

        npos::ipc::Message response;
        bool answered = false;
    };
}

// The services run on a real bus with the real filesystem task, so the whole chain of requests is checked.
TEST_GROUP(Middleware)
{
    mock::RamFlash flash;
    mock::FakeICache icache;
    npos::ipc::SystemBusStatic<4, 16> bus{0};
    fs::task::LittlefsStatic<8, 4> filesystem{2, bus, flash, &icache};
    npos::ipc::ServiceTaskStatic<8, 8, 8> tasks{1, bus};
    npos::ipc::ServiceTaskStatic<8, 8, 4> applicationTask{1, bus};

    etl::array<uint8_t, 16> buffer;
    service::file::Copy fileCopy{tasks, etl::span<uint8_t>(buffer)};
    service::file::Move fileMove{tasks};
    service::file::Read fileRead{tasks};
    service::file::Write fileWrite{tasks};
    // Port and service ids are given by setup(), so the address of fileCopy has to be known up front:
    // tasks is the second port of the bus and fileCopy the first service of it.
    service::dir::Copy dirCopy{tasks, { 1, 0 }};
    service::dir::Move dirMove{tasks};
    service::dir::Remove dirRemove{tasks};
    Client client{applicationTask};
    mock::FakeHost host;
    npos::mdlwr::cli::program::Cp cp{applicationTask};
    npos::mdlwr::cli::program::Mv mv{applicationTask};
    npos::mdlwr::cli::program::Rm rm{applicationTask};

    void setup() override
    {
        bus.addPort(filesystem);
        bus.addPort(tasks);
        bus.addPort(applicationTask);
        tasks.subscribe(fileCopy);
        tasks.subscribe(fileMove);
        tasks.subscribe(fileRead);
        tasks.subscribe(fileWrite);
        tasks.subscribe(dirCopy);
        tasks.subscribe(dirMove);
        tasks.subscribe(dirRemove);
        CHECK_TRUE(fileCopy.getOid() == (npos::ipc::Oid{ 1, 0 }));
        applicationTask.subscribe(client);
        applicationTask.subscribe(cp);
        applicationTask.subscribe(mv);
        applicationTask.subscribe(rm);

        filesystem.initialize();
        tasks.initialize();
        applicationTask.initialize();
    }

    // Runs the tasks until nothing is left to do.
    void runUntilIdle()
    {
        bool worked = true;

        for(int i = 0; worked and i < 100000; i++)
        {
            worked = false;

            for(npos::nps::Task* task : { static_cast<npos::nps::Task*>(&filesystem), static_cast<npos::nps::Task*>(&tasks),
                static_cast<npos::nps::Task*>(&applicationTask) })
            {
                if(task->isReady())
                {
                    task->process();
                    worked = true;
                }
            }
        }
    }

    template<typename T>
    T& send(T& request)
    {
        client.answered = false;
        client.getApi().send(client, request);
        runUntilIdle();
        CHECK_TRUE(client.answered);
        return client.last<T>();
    }

    void makeDirectory(const char* path)
    {
        client.answered = false;
        fs::api::dir::make(client, path);
        runUntilIdle();
        CHECK_EQUAL(fs::msg::Status::SUCCESS, client.last<fs::msg::dir::Make>().getStatus());
    }

    void write(const char* path, etl::string_view text, bool append = false)
    {
        msg::file::Write request{npos::ipc::NULL_OID, path,
            etl::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size()), append};
        CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());
    }

    etl::string<256> read(const char* path, msg::Status expected = msg::Status::SUCCESS)
    {
        uint8_t output[256];
        msg::file::Read request{npos::ipc::NULL_OID, path, etl::span<uint8_t>(output)};
        auto& response = send(request);
        CHECK_EQUAL(expected, response.getStatus());
        return etl::string<256>(reinterpret_cast<const char*>(output), response.getOutputLength());
    }

    // Runs a program until it exits.
    int execute(npos::cli::Program& program, const char* args)
    {
        host.exitCount = 0;
        host.output.clear();
        program.start(host, args);
        runUntilIdle();
        CHECK_EQUAL(1, host.exitCount);
        return host.exitCode;
    }

    bool exists(const char* path)
    {
        fs::Info info;
        client.answered = false;
        fs::api::stat(client, path, info);
        runUntilIdle();
        return client.last<fs::msg::Stat>().getStatus() == fs::msg::Status::SUCCESS;
    }
};

TEST(Middleware, WritesAndReadsFile)
{
    write("/a.txt", "hello");
    write("/a.txt", " world", true);

    STRCMP_EQUAL("hello world", read("/a.txt").c_str());

    write("/a.txt", "x");
    STRCMP_EQUAL("x", read("/a.txt").c_str());
}

TEST(Middleware, ReadReportsTooSmallBufferAndMissingFile)
{
    write("/a.txt", etl::string<100>(100, 'a'));
    uint8_t small[10];
    msg::file::Read request{npos::ipc::NULL_OID, "/a.txt", etl::span<uint8_t>(small)};

    CHECK_EQUAL(msg::Status::BUFFER_TOO_SMALL, send(request).getStatus());

    read("/missing", msg::Status::FILE_NOT_FOUND);
}

TEST(Middleware, CopiesFileLongerThanTheBuffer)
{
    etl::string<100> content;
    for(size_t i = 0; i < 100; i++)
        content.push_back(static_cast<char>('a' + i % 26));
    write("/a.txt", content);

    msg::file::Copy request{npos::ipc::NULL_OID, "/a.txt", "/b.txt"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());
    STRCMP_EQUAL(content.c_str(), read("/b.txt").c_str());
    STRCMP_EQUAL(content.c_str(), read("/a.txt").c_str());
}

TEST(Middleware, CopyOntoItselfKeepsTheContent)
{
    write("/a.txt", "hello");
    msg::file::Copy request{npos::ipc::NULL_OID, "/a.txt", "/a.txt"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());
    STRCMP_EQUAL("hello", read("/a.txt").c_str());
}

TEST(Middleware, MovesFileOverExistingOne)
{
    write("/a.txt", "new");
    write("/b.txt", "old");
    msg::file::Move request{npos::ipc::NULL_OID, "/a.txt", "/b.txt"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());

    STRCMP_EQUAL("new", read("/b.txt").c_str());
    CHECK_FALSE(exists("/a.txt"));
}

TEST(Middleware, MoveOfMissingFileKeepsTheDestination)
{
    write("/b.txt", "old");
    msg::file::Move request{npos::ipc::NULL_OID, "/a.txt", "/b.txt"};

    CHECK_EQUAL(msg::Status::FILE_NOT_FOUND, send(request).getStatus());
    STRCMP_EQUAL("old", read("/b.txt").c_str());
}

TEST(Middleware, CopiesDirectoryTree)
{
    makeDirectory("/src");
    makeDirectory("/src/sub");
    makeDirectory("/src/sub/deep");
    makeDirectory("/src/empty");
    write("/src/a.txt", "a");
    write("/src/b.txt", "b");
    write("/src/sub/c.txt", "c");
    write("/src/sub/deep/d.txt", "d");
    write("/src/z.txt", "z");
    msg::dir::Copy request{npos::ipc::NULL_OID, "/src", "/dst"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());

    STRCMP_EQUAL("a", read("/dst/a.txt").c_str());
    STRCMP_EQUAL("b", read("/dst/b.txt").c_str());
    STRCMP_EQUAL("c", read("/dst/sub/c.txt").c_str());
    STRCMP_EQUAL("d", read("/dst/sub/deep/d.txt").c_str());
    STRCMP_EQUAL("z", read("/dst/z.txt").c_str());
    CHECK_TRUE(exists("/dst/empty"));
    STRCMP_EQUAL("a", read("/src/a.txt").c_str());
}

TEST(Middleware, CopyOfDirectoryOntoExistingOneIsRejected)
{
    makeDirectory("/src");
    makeDirectory("/dst");
    msg::dir::Copy request{npos::ipc::NULL_OID, "/src", "/dst"};

    CHECK_EQUAL(msg::Status::ALREADY_EXISTS, send(request).getStatus());
}

TEST(Middleware, MovesDirectoryTree)
{
    makeDirectory("/src");
    makeDirectory("/src/sub");
    write("/src/sub/c.txt", "c");
    msg::dir::Move request{npos::ipc::NULL_OID, "/src", "/dst"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());

    STRCMP_EQUAL("c", read("/dst/sub/c.txt").c_str());
    CHECK_FALSE(exists("/src"));
}

TEST(Middleware, RemovesDirectoryWithContent)
{
    makeDirectory("/src");
    makeDirectory("/src/sub");
    makeDirectory("/src/sub/deep");
    makeDirectory("/src/empty");
    write("/src/a.txt", "a");
    write("/src/b.txt", "b");
    write("/src/sub/c.txt", "c");
    write("/src/sub/deep/d.txt", "d");
    write("/keep.txt", "k");
    msg::dir::Remove request{npos::ipc::NULL_OID, "/src"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());

    CHECK_FALSE(exists("/src"));
    STRCMP_EQUAL("k", read("/keep.txt").c_str());
}

TEST(Middleware, RemovesEmptyDirectory)
{
    makeDirectory("/src");
    msg::dir::Remove request{npos::ipc::NULL_OID, "/src"};

    CHECK_EQUAL(msg::Status::SUCCESS, send(request).getStatus());
    CHECK_FALSE(exists("/src"));
}

TEST(Middleware, RemoveOfFileOrMissingDirectoryIsReported)
{
    write("/a.txt", "a");
    msg::dir::Remove file{npos::ipc::NULL_OID, "/a.txt"};
    msg::dir::Remove missing{npos::ipc::NULL_OID, "/missing"};

    CHECK_EQUAL(msg::Status::NOT_A_DIRECTORY, send(file).getStatus());
    CHECK_EQUAL(msg::Status::DIR_NOT_FOUND, send(missing).getStatus());
}

TEST(Middleware, CpCopiesFileAndDirectoryTree)
{
    makeDirectory("/src");
    makeDirectory("/src/sub");
    makeDirectory("/dst");
    write("/a.txt", "a");
    write("/src/b.txt", "b");
    write("/src/sub/c.txt", "c");

    CHECK_EQUAL(0, execute(cp, "a.txt /dst"));
    CHECK_EQUAL(1, execute(cp, "src /dst"));
    CHECK_TRUE(host.outputIs("cp: -r not specified; omitting directory 'src'\r\n"));
    CHECK_EQUAL(0, execute(cp, "-r src /copy"));
    CHECK_EQUAL(0, execute(cp, "-r src /dst"));

    STRCMP_EQUAL("a", read("/dst/a.txt").c_str());
    STRCMP_EQUAL("c", read("/copy/sub/c.txt").c_str());
    STRCMP_EQUAL("b", read("/dst/src/b.txt").c_str());
    STRCMP_EQUAL("a", read("/a.txt").c_str());
}

TEST(Middleware, MvMovesFilesAndDirectories)
{
    makeDirectory("/src");
    makeDirectory("/dst");
    write("/a.txt", "a");
    write("/src/b.txt", "b");

    CHECK_EQUAL(0, execute(mv, "a.txt /dst"));
    CHECK_EQUAL(0, execute(mv, "src /dst"));

    STRCMP_EQUAL("a", read("/dst/a.txt").c_str());
    STRCMP_EQUAL("b", read("/dst/src/b.txt").c_str());
    CHECK_FALSE(exists("/a.txt"));
    CHECK_FALSE(exists("/src"));
    CHECK_EQUAL(1, execute(mv, "missing /dst"));
}

TEST(Middleware, RmRemovesFilesAndDirectoriesWithContent)
{
    makeDirectory("/src");
    makeDirectory("/src/sub");
    write("/a.txt", "a");
    write("/src/sub/c.txt", "c");

    CHECK_EQUAL(1, execute(rm, "src"));
    CHECK_TRUE(exists("/src"));
    CHECK_EQUAL(0, execute(rm, "a.txt"));
    CHECK_EQUAL(0, execute(rm, "-r src"));

    CHECK_FALSE(exists("/a.txt"));
    CHECK_FALSE(exists("/src"));
    CHECK_EQUAL(1, execute(rm, "a.txt"));
    CHECK_EQUAL(0, execute(rm, "-f a.txt"));
}
