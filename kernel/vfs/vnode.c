#include <kernel/vfs/vnode.h>
#include <kernel/kmem.h>
#include <string.h>

struct vnode* vnode_alloc(uint32_t type, struct vnode_ops *ops) {
    struct vnode *vn = kmalloc(sizeof(struct vnode));
    if (!vn) return NULL;

    memset(vn, 0, sizeof(struct vnode));
    vn->ref_count = 1;
    vn->type = type;
    vn->ops = ops;
    spin_lock_init(&vn->lock);

    return vn;
}

void vput(struct vnode *vn) {
    if (!vn) return;

    if (__atomic_fetch_sub(&vn->ref_count, 1, __ATOMIC_SEQ_CST) == 1) {
        // TODO
        kfree(vn);
    }
}