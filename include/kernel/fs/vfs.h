#pragma once

#include <kernel/fs/vnode.h>

extern struct vnode *g_root_vnode;

void vfs_init(void);

int vfs_lookup(const char *path, struct vnode *base, struct vnode **vpp);
int vfs_open(const char *path, int flags, mode_t mode, int *fd);
int vfs_close(int fd);
int vfs_read(int fd, void *buf, size_t n);
int vfs_write(int fd, const void *buf, size_t n);
int vfs_lseek(int fd, off_t offset, int whence);
int vfs_readdir(int fd, void *buf, size_t count);