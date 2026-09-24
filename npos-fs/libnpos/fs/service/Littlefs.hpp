#pragma once

/**
 * @file Littlefs.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <liblittlefs/lfs.h>

#include <libnpos/os/Service.hpp>
#include <libnpos/os/filesystem/message/File.hpp>
#include <libnpos/os/filesystem/message/System.hpp>
#include <libnpos/os/filesystem/message/Directory.hpp>
#include <libnpos/fs/hal/Flash.hpp>
#include <libnpos/fs/hal/ICache.hpp>

namespace npos::fs::service
{
    class Littlefs : public os::Service
    {
    public:
        static constexpr size_t READ_SIZE = 256;

        struct FileContext
        {
            int fd;
            lfs_file_t file;
        };

        struct DirectoryContext
        {
            int dd;
            lfs_dir_t dir;
        };

        using FileStorage = etl::ivector<FileContext>;
        using DirectoryStorage = etl::ivector<DirectoryContext>;

        Littlefs(hal::Flash& flash, SystemBus& bus, hal::ICache* icache, 
            FileStorage& storage, DirectoryStorage& directoryStorage);

        void initalize() override;

        void onReceive(const ipc::Message&) override;
        bool accepts(ipc::Message::Id) const override;

        //Littlefs flash operations
        int readFlash(lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
        int programFlash(lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
        int eraseFlash(lfs_block_t block);
        int syncFlash();

    protected:
        void processFileOpen(const os::filesystem::message::FileOpenRequest&);
        void processFileClose(const os::filesystem::message::FileCloseRequest&);
        void processFileRead(const os::filesystem::message::FileReadRequest&);
        void processFileWrite(const os::filesystem::message::FileWriteRequest&);
        void processFileSync(const os::filesystem::message::FileSyncRequest&);
        void processFileSeek(const os::filesystem::message::FileSeekRequest&);
        void processFileTruncate(const os::filesystem::message::FileTruncateRequest&);
        void processFileTell(const os::filesystem::message::FileTellRequest&);

        void processDirMake(const os::filesystem::message::DirectoryMakeRequest&);
        void processDirOpen(const os::filesystem::message::DirectoryOpenRequest&);
        void processDirClose(const os::filesystem::message::DirectoryCloseRequest&);
        void processDirRead(const os::filesystem::message::DirectoryReadRequest&);
        void processDirSeek(const os::filesystem::message::DirectorySeekRequest&);
        void processDirTell(const os::filesystem::message::DirectoryTellRequest&);

        void processSystemFormat(const os::filesystem::message::SystemFormatRequest&);
        void processSystemMount(const os::filesystem::message::SystemMountRequest&);
        void processSystemUnmount(const os::filesystem::message::SystemUnmountRequest&);
        void processSystemRemove(const os::filesystem::message::SystemRemoveRequest&);
        void processSystemRename(const os::filesystem::message::SystemRenameRequest&);
        void processSystemStatfs(const os::filesystem::message::SystemStatfsRequest&);

        etl::optional<FileContext> findFileContext(int fd);
        etl::optional<DirectoryContext> findDirectoryContext(int dd);
        void removeFileContext(int fd);
        void removeDirectoryContext(int dd);

    private:
        lfs_t lfs;
        lfs_config config;
        hal::Flash& flash;
        hal::ICache* icache;
        FileStorage& fileStorage;
        DirectoryStorage& directoryStorage;
        int nextFileDescriptor;
        int nextDirectoryDescriptor;
    };
}