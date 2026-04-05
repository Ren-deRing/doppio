#include <kernel/proc.h>
#include <kernel/kmem.h>
#include <kernel/cpu.h>
#include <kernel/init.h>
#include <kernel/sched.h>

#include <uapi/errno.h>

#include <string.h>

struct proc* proc_create(pid_t pid) {
    struct proc *p = kmalloc(sizeof(struct proc));
    if (!p) return NULL;

    memset(p, 0, sizeof(struct proc));
    p->p_pid = pid;
    
    extern struct vnode *g_root_vnode;
    if (g_root_vnode) {
        vref(g_root_vnode);
        p->p_cwd = g_root_vnode;
    }

    return p;
}

struct thread* thread_create(struct proc *p, tid_t tid, void (*entry)(void)) {
    struct thread *t = kmalloc(sizeof(struct thread));
    if (!t) return NULL;

    memset(t, 0, sizeof(struct thread));
    
    t->t_kstack = kmalloc(KSTACK_SIZE);
    if (!t->t_kstack) {
        kfree(t);
        return NULL;
    }

    t->t_tid = tid;
    t->t_proc = p;
    t->t_state = 0; // READY
    t->t_ticks = 0;
    t->t_need_resched = false;

    arch_thread_setup(t, entry);
    
    return t;
}

void proc_free(struct proc *p) {
    if (!p) return;

    for (int i = 0; i < MAX_FILES; i++) {
        if (p->p_fd_table[i]) {
            file_close(p->p_fd_table[i]);
            p->p_fd_table[i] = NULL;
        }
    }

    if (p->p_cwd) {
        vput(p->p_cwd);
    }

    kfree(p);
}

int proc_alloc_fd(struct proc *p, struct file *f) {
    if (!p || !f) return -EINVAL;

    spin_lock(&p->p_lock); // 락 획득
    for (int i = 0; i < MAX_FILES; i++) {
        if (p->p_fd_table[i] == NULL) {
            p->p_fd_table[i] = f;
            spin_unlock(&p->p_lock);
            return i;
        }
    }
    spin_unlock(&p->p_lock);

    return -EMFILE; // 빈자리가 없음!!
}