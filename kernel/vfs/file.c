#include <kernel/fs/file.h>
#include <kernel/kmem.h>
#include <string.h>

struct file *file_alloc(void) {
    struct file *f = kmalloc(sizeof(struct file));
    if (!f) return NULL;

    memset(f, 0, sizeof(struct file));
    f->f_refcnt = 1;
    spin_lock_init(&f->f_lock);

    return f;
}

void file_ref(struct file *f) {
    if (f) {
        __atomic_fetch_add(&f->f_refcnt, 1, __ATOMIC_SEQ_CST);
    }
}

void file_close(struct file *f) {
    if (!f) return;

    if (__atomic_fetch_sub(&f->f_refcnt, 1, __ATOMIC_SEQ_CST) == 1) {
        if (f->f_vn) {
            vput(f->f_vn);
        }
        kfree(f);
    }
}