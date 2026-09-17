#pragma once

/**
 * @file Id.hpp
 * @author Adrian Szczepanski
 * @date 17-09-2026
 */

#include <libnpos/os/message/Id.hpp>

namespace npos::os::filesystem::message
{
    enum Id : ipc::Message::Id
    {
        FILE_OPEN_REQUEST = os::message::Id::FILESYSTEM,
        FILE_OPEN_RESPONSE,
        FILE_CLOSE_REQUEST,
        FILE_CLOSE_RESPONSE,
        FILE_WRITE_REQUEST,
        FILE_WRITE_RESPONSE,
        FILE_READ_REQUEST,
        FILE_READ_RESPONSE,
        FILE_SYNC_REQUEST,
        FILE_SYNC_RESPONSE,
        FILE_SEEK_REQUEST,
        FILE_SEEK_RESPONSE,
        FILE_TRUNCATE_REQUEST,
        FILE_TRUNCATE_RESPONSE,
        FILE_TELL_REQUEST,
        FILE_TELL_RESPONSE,

        DIR_MAKE_REQUEST,
        DIR_MAKE_RESPONSE,
        DIR_OPEN_REQUEST,
        DIR_OPEN_RESPONSE,
        DIR_CLOSE_REQUEST,
        DIR_CLOSE_RESPONSE,
        DIR_READ_REQUEST,
        DIR_READ_RESPONSE,
        DIR_SEEK_REQUEST,
        DIR_SEEK_RESPONSE,
        DIR_TELL_REQUEST,
        DIR_TELL_RESPONSE,

        SYSTEM_FORMAT_REQUEST,
        SYSTEM_FORMAT_RESPONSE,
        SYSTEM_MOUNT_REQUEST,
        SYSTEM_MOUNT_RESPONSE,
        SYSTEM_UNMOUNT_REQUEST,
        SYSTEM_UNMOUNT_RESPONSE,
        SYSTEM_REMOVE_REQUEST,
        SYSTEM_REMOVE_RESPONSE,
        SYSTEM_RENAME_REQUEST,
        SYSTEM_RENAME_RESPONSE,
        SYSTEM_STATFS_REQUEST,
        SYSTEM_STATFS_RESPONSE
    };
}

// int ph_lfs_mount(lfs_t *lfs, const struct lfs_config *cfg, unsigned int port);
// int ph_lfs_unmount(lfs_t *lfs);
// int ph_lfs_create(lfs_t *lfs, id_t parentPhId, const char *name, uint16_t mode, oid_t *dev, id_t *result);
// int ph_lfs_open(lfs_t *lfs, id_t phId);
// int ph_lfs_close(lfs_t *lfs, id_t phId);
// ssize_t ph_lfs_write(lfs_t *lfs, id_t phId, size_t offs, const void *data, size_t len);
// ssize_t ph_lfs_read(lfs_t *lfs, id_t phId, size_t offs, void *data, size_t len);
// int ph_lfs_sync(lfs_t *lfs, id_t phId);
// int ph_lfs_truncate(lfs_t *lfs, id_t phId, size_t size);
// ssize_t ph_lfs_lookup(lfs_t *lfs, id_t parentPhId, const char *path, id_t *res, oid_t *dev);
// int ph_lfs_getattr(lfs_t *lfs, id_t phId, int type, long long *attr);
// int ph_lfs_getattrAll(lfs_t *lfs, id_t phId, struct _attrAll *attrs);
// int ph_lfs_setattr(lfs_t *lfs, id_t phId, int type, long long attr, const void *data, size_t size);
// int ph_lfs_readdir(lfs_t *lfs, id_t phId, size_t offs, struct dirent *dent, size_t size);
// int ph_lfs_rename(lfs_t *lfs, id_t dir, id_t source, const char *newName);
// int ph_lfs_unlink(lfs_t *lfs, id_t dir, const char *name);
// int ph_lfs_destroy(lfs_t *lfs, id_t phId);
// int ph_lfs_statfs(lfs_t *lfs, struct statvfs *st);