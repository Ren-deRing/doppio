#include <kernel/fs/vnode.h>
#include <kernel/kmem.h>

#include <uapi/errno.h>

#include <string.h>

static LIST_HEAD(vnode_all_list);
static LIST_HEAD(vnode_free_list);

static spinlock_t vnode_list_lock = SPINLOCK_INITIALIZER;
static struct list_node vnode_hash[VNODE_HASH_SIZE];

void vnode_init(void) {
    for (int i = 0; i < VNODE_HASH_SIZE; i++) {
        list_init(&vnode_hash[i]);
    }
}

static uint32_t vnode_hash_func(struct vnode *dvp, const char *name) {
    uint32_t hash = (uintptr_t)dvp;
    while (*name) {
        hash = ((hash << 5) + hash) + *name++;
    }
    return hash % VNODE_HASH_SIZE;
}

struct vnode* vnode_alloc(uint32_t type, struct vnode_ops *ops) {
    struct vnode *vn = NULL;

    spin_lock(&vnode_list_lock);
    if (!list_empty(&vnode_free_list)) {
        vn = list_first_entry(&vnode_free_list, struct vnode, v_freelist);
        list_del(&vn->v_freelist);
        
        if (vn->v_hash.next && vn->v_hash.next != &vn->v_hash) {
            list_del(&vn->v_hash);
        }
    }
    spin_unlock(&vnode_list_lock);

    if (!vn) {
        vn = kmalloc(sizeof(struct vnode));
        if (!vn) return NULL;
        
        spin_lock(&vnode_list_lock);
        list_add(&vn->v_all, &vnode_all_list);
        spin_unlock(&vnode_list_lock);
    }

    vn->ref_count = 1;
    vn->type = type;
    vn->ops = ops;
    vn->data = NULL;
    vn->mnt = NULL;
    vn->v_parent = NULL;
    memset(vn->v_name, 0, sizeof(vn->v_name));

    list_init(&vn->v_freelist);
    list_init(&vn->v_hash);
    spin_lock_init(&vn->lock);

    return vn;
}

int vfs_cached_lookup(struct vnode *dvp, const char *name, struct vnode **vpp) {
    uint32_t h = vnode_hash_func(dvp, name);
    struct vnode *vp;

    spin_lock(&vnode_list_lock);
    
    list_for_each_entry(vp, &vnode_hash[h], v_hash) {
        if (vp->v_parent == dvp && strcmp(vp->v_name, name) == 0) {
            if (vp->ref_count == 0) {
                list_del(&vp->v_freelist);
            }
            __atomic_fetch_add(&vp->ref_count, 1, __ATOMIC_SEQ_CST);
            *vpp = vp;
            spin_unlock(&vnode_list_lock);
            return 0;
        }
    }
    
    spin_unlock(&vnode_list_lock);
    return -ENOENT; // 캐시에 없음
}

void vfs_hash_insert(struct vnode *dvp, const char *name, struct vnode *vp) {
    uint32_t h = vnode_hash_func(dvp, name);

    spin_lock(&vnode_list_lock);
    
    if (vp->v_hash.next && vp->v_hash.next != &vp->v_hash) {
        list_del(&vp->v_hash);
    }

    vp->v_parent = dvp;
    strncpy(vp->v_name, name, sizeof(vp->v_name) - 1);
    vp->v_name[sizeof(vp->v_name) - 1] = '\0';
    
    list_add(&vp->v_hash, &vnode_hash[h]);
    
    spin_unlock(&vnode_list_lock);
}

void vput(struct vnode *vn) {
    if (!vn) return;

    int release = 0;
    spin_lock(&vnode_list_lock);
    
    if (__atomic_sub_fetch(&vn->ref_count, 1, __ATOMIC_SEQ_CST) == 0) {
        list_add_tail(&vn->v_freelist, &vnode_free_list);
        release = 1;
    }
    spin_unlock(&vnode_list_lock);

    if (release && vn->ops && vn->ops->inactive) {
        vn->ops->inactive(vn);
    }
}

int vget(struct vnode *vp) {
    spin_lock(&vnode_list_lock);

    if (vp->ref_count == 0) {
        list_del(&vp->v_freelist);
    }
    
    __atomic_fetch_add(&vp->ref_count, 1, __ATOMIC_SEQ_CST);
    
    spin_unlock(&vnode_list_lock);
    return 0;
}