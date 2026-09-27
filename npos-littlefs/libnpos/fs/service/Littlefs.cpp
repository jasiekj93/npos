#include "Littlefs.hpp"
#include <libnpos/os/syslog/Stream.hpp>
#include <libnpos/fs/FlagConverter.hpp>
#include <libnpos/fs/StatusConverter.hpp>
#include <libnpos/fs/InfoConverter.hpp>

using namespace npos;
using namespace npos::fs;
using namespace npos::fs::service;

Littlefs::Littlefs(hal::Flash& flash, SystemBus& bus, hal::ICache* icache, 
        FileStorage& fileStorage, DirectoryStorage& directoryStorage)
    : os::Service(bus)
    , flash(flash)
    , icache(icache)
    , fileStorage(fileStorage)
    , directoryStorage(directoryStorage)
    , nextFileDescriptor(1)
    , nextDirectoryDescriptor(1)
{
}

void Littlefs::initalize()
{
    config.read = [](const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) -> int
    {
        auto fs = reinterpret_cast<Littlefs*>(c->context);
        return fs->readFlash(block, off, buffer,size);
    };

    config.prog = [](const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) -> int
    {
        auto fs = reinterpret_cast<Littlefs*>(c->context);
        return fs->programFlash(block, off, buffer, size);
    };

    config.erase = [](const struct lfs_config *c, lfs_block_t block) -> int
    {
        auto fs = reinterpret_cast<Littlefs*>(c->context);
        return fs->eraseFlash(block);
    };

    config.sync = [](const struct lfs_config *c) -> int
    {
        auto fs = reinterpret_cast<Littlefs*>(c->context);
        return fs->syncFlash();
    };

    config.read_size = READ_SIZE;
    config.prog_size = flash.getProgramSize();
    config.block_size = flash.getBlockSize();
    config.block_count = flash.getBlockCount();
    config.cache_size = READ_SIZE;
    config.lookahead_size = READ_SIZE;
    config.block_cycles = flash.getBlockCycles();
    config.context = this;
}

void Littlefs::onReceive(const ipc::Message& message)
{
    syslog(os::syslog::Level::DEBUG) << "Received Filesystem Request: " << static_cast<uint8_t>(message.getId()) << os::syslog::endl;

    switch (message.getId())
    {
    case os::filesystem::message::Id::FILE_OPEN_REQUEST:
        return processFileOpen(static_cast<const os::filesystem::message::FileOpenRequest&>(message));
    case os::filesystem::message::Id::FILE_CLOSE_REQUEST:
        return processFileClose(static_cast<const os::filesystem::message::FileCloseRequest&>(message));
    case os::filesystem::message::Id::FILE_READ_REQUEST:
        return processFileRead(static_cast<const os::filesystem::message::FileReadRequest&>(message));
    case os::filesystem::message::Id::FILE_WRITE_REQUEST:
        return processFileWrite(static_cast<const os::filesystem::message::FileWriteRequest&>(message));
    case os::filesystem::message::Id::FILE_SYNC_REQUEST:
        return processFileSync(static_cast<const os::filesystem::message::FileSyncRequest&>(message));
    case os::filesystem::message::Id::FILE_SEEK_REQUEST:
        return processFileSeek(static_cast<const os::filesystem::message::FileSeekRequest&>(message));
    case os::filesystem::message::Id::FILE_TRUNCATE_REQUEST:
        return processFileTruncate(static_cast<const os::filesystem::message::FileTruncateRequest&>(message));
    case os::filesystem::message::Id::FILE_TELL_REQUEST:
        return processFileTell(static_cast<const os::filesystem::message::FileTellRequest&>(message));

    case os::filesystem::message::Id::DIR_MAKE_REQUEST:
        return processDirMake(static_cast<const os::filesystem::message::DirectoryMakeRequest&>(message));
    case os::filesystem::message::Id::DIR_OPEN_REQUEST:
        return processDirOpen(static_cast<const os::filesystem::message::DirectoryOpenRequest&>(message));
    case os::filesystem::message::Id::DIR_CLOSE_REQUEST:
        return processDirClose(static_cast<const os::filesystem::message::DirectoryCloseRequest&>(message));
    case os::filesystem::message::Id::DIR_READ_REQUEST:
        return processDirRead(static_cast<const os::filesystem::message::DirectoryReadRequest&>(message));
    case os::filesystem::message::Id::DIR_SEEK_REQUEST:
        return processDirSeek(static_cast<const os::filesystem::message::DirectorySeekRequest&>(message));
    case os::filesystem::message::Id::DIR_TELL_REQUEST:
        return processDirTell(static_cast<const os::filesystem::message::DirectoryTellRequest&>(message));

    case os::filesystem::message::Id::SYSTEM_FORMAT_REQUEST:
        return processSystemFormat(static_cast<const os::filesystem::message::SystemFormatRequest&>(message));
    case os::filesystem::message::Id::SYSTEM_MOUNT_REQUEST:
        return processSystemMount(static_cast<const os::filesystem::message::SystemMountRequest&>(message));
    case os::filesystem::message::Id::SYSTEM_UNMOUNT_REQUEST:
        return processSystemUnmount(static_cast<const os::filesystem::message::SystemUnmountRequest&>(message));
    case os::filesystem::message::Id::SYSTEM_REMOVE_REQUEST:
        return processSystemRemove(static_cast<const os::filesystem::message::SystemRemoveRequest&>(message));
    case os::filesystem::message::Id::SYSTEM_RENAME_REQUEST:
        return processSystemRename(static_cast<const os::filesystem::message::SystemRenameRequest&>(message));
    case os::filesystem::message::Id::SYSTEM_STATFS_REQUEST:
        return processSystemStatfs(static_cast<const os::filesystem::message::SystemStatfsRequest&>(message));
    default:
        return;
    }
}

bool Littlefs::accepts(ipc::Message::Id id) const
{
    switch (id)
    {
    case os::filesystem::message::Id::FILE_OPEN_REQUEST:
    case os::filesystem::message::Id::FILE_CLOSE_REQUEST:
    case os::filesystem::message::Id::FILE_READ_REQUEST:
    case os::filesystem::message::Id::FILE_WRITE_REQUEST:
    case os::filesystem::message::Id::FILE_SYNC_REQUEST:
    case os::filesystem::message::Id::FILE_SEEK_REQUEST:
    case os::filesystem::message::Id::FILE_TRUNCATE_REQUEST:
    case os::filesystem::message::Id::FILE_TELL_REQUEST:

    case os::filesystem::message::Id::DIR_MAKE_REQUEST:
    case os::filesystem::message::Id::DIR_OPEN_REQUEST:
    case os::filesystem::message::Id::DIR_CLOSE_REQUEST:
    case os::filesystem::message::Id::DIR_READ_REQUEST:
    case os::filesystem::message::Id::DIR_SEEK_REQUEST:
    case os::filesystem::message::Id::DIR_TELL_REQUEST:

    case os::filesystem::message::Id::SYSTEM_FORMAT_REQUEST:
    case os::filesystem::message::Id::SYSTEM_MOUNT_REQUEST:
    case os::filesystem::message::Id::SYSTEM_UNMOUNT_REQUEST:
    case os::filesystem::message::Id::SYSTEM_REMOVE_REQUEST:
    case os::filesystem::message::Id::SYSTEM_RENAME_REQUEST:
    case os::filesystem::message::Id::SYSTEM_STATFS_REQUEST:
        return true;
    default:
        return false;
    }
}

int Littlefs::readFlash(lfs_block_t block, lfs_off_t offset, void *buffer, lfs_size_t size)
{
    size_t address = flash.getStartAddress() + (block * flash.getBlockSize()) + offset;
    flash.read(address, static_cast<uint8_t*>(buffer), size);

    return LFS_ERR_OK;
}

int Littlefs::programFlash(lfs_block_t block, lfs_off_t offset, const void *buff, lfs_size_t size)
{
    size_t address = flash.getStartAddress() + (block * flash.getBlockSize()) + offset;
    auto buffer = reinterpret_cast<const uint8_t*>(buff);

    if(icache != nullptr)
        if(not icache->disable())
            return LFS_ERR_IO;

    if(not flash.unlock())
        return LFS_ERR_IO;

    for(size_t i = 0; i < size; i += flash.getProgramSize())
    {
        if(not flash.waitForLastOperation(0x10000))
        {
            flash.lock();
            return LFS_ERR_IO;
        }

        if(not flash.program(address, buffer))
        {
            flash.lock();
            return LFS_ERR_IO;
        }

        address += flash.getProgramSize();
        buffer += flash.getProgramSize();
    }

    if(not flash.lock())
        return LFS_ERR_IO;

    if(icache != nullptr)
        if(not icache->enable())
            return LFS_ERR_IO;

    return LFS_ERR_OK;
}

int Littlefs::eraseFlash(lfs_block_t block)
{
    if (not flash.unlock())
        return LFS_ERR_IO;

    if(not flash.eraseBlocks(block, 1))
    {
        flash.lock();
        return LFS_ERR_IO;
    }

    if(not flash.lock())
        return LFS_ERR_IO;

    return LFS_ERR_OK;
}

int Littlefs::syncFlash()
{
    if(not flash.waitForLastOperation(0x10000))
    {
        flash.lock();
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

void Littlefs::processFileOpen(const os::filesystem::message::FileOpenRequest& message)
{
    os::filesystem::message::FileOpenResponse response;
    
    if(fileStorage.full())
    {
        response.status = os::filesystem::Status::TOO_MANY_FILES_OPENED;
        response.fileDescriptor = -1;
        response.fileSize = 0;
        return sendTo(message.senderId, response);
    }

    FileContext newContext;
    auto status = lfs_file_open(&lfs, &newContext.file, message.path.c_str(), osToLfsFlags(message.flags));

    if(status != LFS_ERR_OK)
    {
        response.status = lfsToOsStatus(status);
        response.fileDescriptor = -1;
        response.fileSize = 0;
        return sendTo(message.senderId, response);
    }

    newContext.fd = nextFileDescriptor++;
    fileStorage.push_back(newContext);

    response.status = os::filesystem::Status::OK;
    response.fileDescriptor = newContext.fd;
    response.fileSize = lfs_file_size(&lfs, &newContext.file);
    sendTo(message.senderId, response);
}

void Littlefs::processFileClose(const os::filesystem::message::FileCloseRequest& message)
{
    os::filesystem::message::FileCloseResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_close(&lfs, &context->file);
    if(status != LFS_ERR_OK)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    removeFileContext(context->fd);
    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processFileRead(const os::filesystem::message::FileReadRequest& message)
{
    os::filesystem::message::FileReadResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        response.bytesRead = 0;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_read(&lfs, &context->file, message.buffer.data(), message.buffer.size());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        response.bytesRead = 0;
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.bytesRead = status;
    sendTo(message.senderId, response);
}

void Littlefs::processFileWrite(const os::filesystem::message::FileWriteRequest& message)
{
    os::filesystem::message::FileWriteResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        response.bytesWritten = 0;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_write(&lfs, &context->file, message.data.data(), message.data.size());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        response.bytesWritten = 0;
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.bytesWritten = status;
    sendTo(message.senderId, response);
}

void Littlefs::processFileSync(const os::filesystem::message::FileSyncRequest& message)
{
    os::filesystem::message::FileSyncResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_sync(&lfs, &context->file);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processFileSeek(const os::filesystem::message::FileSeekRequest& message)
{
    os::filesystem::message::FileSeekResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_seek(&lfs, &context->file, message.offset, message.whence);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        response.fileOffset = 0;
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.fileOffset = status;
    sendTo(message.senderId, response);
}

void Littlefs::processFileTruncate(const os::filesystem::message::FileTruncateRequest& message)
{
    os::filesystem::message::FileTruncateResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_truncate(&lfs, &context->file, message.size);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processFileTell(const os::filesystem::message::FileTellRequest& message)
{
    os::filesystem::message::FileTellResponse response;

    auto context = findFileContext(message.fileDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::FILE_NOT_FOUND;
        response.fileOffset = 0;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_file_tell(&lfs, &context->file);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        response.fileOffset = 0;
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.fileOffset = status;
    sendTo(message.senderId, response);
}

void Littlefs::processDirMake(const os::filesystem::message::DirectoryMakeRequest& message)
{
    os::filesystem::message::DirectoryMakeResponse response;

    auto status = lfs_mkdir(&lfs, message.path.c_str());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processDirOpen(const os::filesystem::message::DirectoryOpenRequest& message)
{
    os::filesystem::message::DirectoryOpenResponse response;

    lfs_dir_t dir;
    auto status = lfs_dir_open(&lfs, &dir, message.path.c_str());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processDirClose(const os::filesystem::message::DirectoryCloseRequest& message)
{
    os::filesystem::message::DirectoryCloseResponse response;

    auto context = findDirectoryContext(message.directoryDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::DIRECTORY_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_dir_close(&lfs, &context->dir);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    removeDirectoryContext(message.directoryDescriptor);

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processDirRead(const os::filesystem::message::DirectoryReadRequest& message)
{
    os::filesystem::message::DirectoryReadResponse response;

    auto context = findDirectoryContext(message.directoryDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::DIRECTORY_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    lfs_info info;
    auto status = lfs_dir_read(&lfs, &context->dir, &info);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        response.info = lfsToOsInfo(info);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.info = lfsToOsInfo(info);
    sendTo(message.senderId, response);
}

void Littlefs::processDirSeek(const os::filesystem::message::DirectorySeekRequest& message)
{
    os::filesystem::message::DirectorySeekResponse response;

    auto context = findDirectoryContext(message.directoryDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::DIRECTORY_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_dir_seek(&lfs, &context->dir, message.offset);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processDirTell(const os::filesystem::message::DirectoryTellRequest& message)
{
    os::filesystem::message::DirectoryTellResponse response;

    auto context = findDirectoryContext(message.directoryDescriptor);

    if(not context.has_value())
    {
        response.status = os::filesystem::Status::DIRECTORY_NOT_FOUND;
        return sendTo(message.senderId, response);
    }

    auto status = lfs_dir_tell(&lfs, &context->dir);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.position = status;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemFormat(const os::filesystem::message::SystemFormatRequest& message)
{
    os::filesystem::message::SystemFormatResponse response;

    auto status = lfs_format(&lfs, &config);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemMount(const os::filesystem::message::SystemMountRequest& message)
{
    os::filesystem::message::SystemMountResponse response;

    auto status = lfs_mount(&lfs, &config);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemUnmount(const os::filesystem::message::SystemUnmountRequest& message)
{
    os::filesystem::message::SystemUnmountResponse response;

    auto status = lfs_unmount(&lfs);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemRemove(const os::filesystem::message::SystemRemoveRequest& message)
{
    os::filesystem::message::SystemRemoveResponse response;

    auto status = lfs_remove(&lfs, message.path.c_str());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemRename(const os::filesystem::message::SystemRenameRequest& message)
{
    os::filesystem::message::SystemRenameResponse response;

    auto status = lfs_rename(&lfs, message.oldPath.c_str(), message.newPath.c_str());
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    sendTo(message.senderId, response);
}

void Littlefs::processSystemStatfs(const os::filesystem::message::SystemStatfsRequest& message)
{
    os::filesystem::message::SystemStatfsResponse response;

    lfs_info info;
    auto status = lfs_stat(&lfs, message.path.c_str(), &info);
    if(status < 0)
    {
        response.status = lfsToOsStatus(status);
        return sendTo(message.senderId, response);
    }

    response.status = os::filesystem::Status::OK;
    response.info = lfsToOsInfo(info);
    sendTo(message.senderId, response);
}

etl::optional<Littlefs::FileContext> Littlefs::findFileContext(int fd)
{
    auto it = etl::find_if(fileStorage.begin(), fileStorage.end(), [&](const FileContext& ctx) {
        return ctx.fd == fd;
    });

    if(it == fileStorage.end())
        return etl::optional<FileContext>();

    return etl::optional<FileContext>(*it);
}

etl::optional<Littlefs::DirectoryContext> Littlefs::findDirectoryContext(int dd)
{
    auto it = etl::find_if(directoryStorage.begin(), directoryStorage.end(), [&](const DirectoryContext& ctx) {
        return ctx.dd == dd;
    });

    if(it == directoryStorage.end())
        return etl::optional<DirectoryContext>();

    return etl::optional<DirectoryContext>(*it);
}

void Littlefs::removeFileContext(int fd)
{
    auto it = etl::find_if(fileStorage.begin(), fileStorage.end(), [&](const FileContext& ctx) {
        return ctx.fd == fd;
    });

    if(it != fileStorage.end())
    {
        auto index = it - fileStorage.begin();
        fileStorage[index] = fileStorage.back();
        fileStorage.pop_back();
    }
}

void Littlefs::removeDirectoryContext(int dd)
{
    auto it = etl::find_if(directoryStorage.begin(), directoryStorage.end(), [&](const DirectoryContext& ctx) {
        return ctx.dd == dd;
    });

    if(it != directoryStorage.end())
    {
        auto index = it - directoryStorage.begin();
        directoryStorage[index] = directoryStorage.back();
        directoryStorage.pop_back();
    }
}
