#pragma once

#include <kernel/fs/vnode.h>
#include <kernel/lock.h>
#include <uapi/types.h>

struct file {
    struct vnode *f_vn;       /* vnode */
    uint32_t      f_flags;    /* 열기 모드 */
    off_t         f_pos;      /* 현재 io 위치 */
    uint32_t      f_refcnt;   /* 참조 카운트 */
    spinlock_t    f_lock;
};

struct file *file_alloc(void);
void file_ref(struct file *f);
void file_close(struct file *f);