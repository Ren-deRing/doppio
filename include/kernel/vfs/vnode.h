#pragma once

#include <uapi/sys/stat.h>
#include <kernel/lock.h>

struct vnode;

struct vnode_ops {
    int (*lookup)(struct vnode *dvp, const char *name, struct vnode **vpp);
    int (*create)(struct vnode *dvp, const char *name, mode_t mode, struct vnode **vpp);
    int (*open)(struct vnode *vp, int flags);
    int (*close)(struct vnode *vp);
    ssize_t (*read)(struct vnode *vp, void *buf, size_t n, off_t off);
    ssize_t (*write)(struct vnode *vp, const void *buf, size_t n, off_t off);
    int (*getattr)(struct vnode *vp, struct stat *st);
    int (*setattr)(struct vnode *vp, struct stat *st);
    int (*readdir)(struct vnode *vp, void *dirent_buf, size_t count, off_t *off);
};

struct vnode {
    uint32_t          ref_count;
    uint32_t          type;
    struct vnode_ops *ops;
    struct mount     *mnt; // 소속된 마운트
    void             *data;
    spinlock_t        lock;
};

static inline void vref(struct vnode *vn) {
    if (vn) __atomic_fetch_add(&vn->ref_count, 1, __ATOMIC_SEQ_CST);
}

void vput(struct vnode *vn);