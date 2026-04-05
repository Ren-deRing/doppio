#include <kernel/proc.h>
#include <kernel/kmem.h>

#include <uapi/errno.h>

#include <string.h>

struct thread *curthread = NULL;

struct proc* proc_create(pid_t pid) {
    struct proc *p = kmalloc(sizeof(struct proc));
    if (!p) return NULL;

    memset(p, 0, sizeof(struct proc));
    p->p_pid = pid;
    
    return p;
}

struct thread* thread_create(struct proc *p, tid_t tid) {
    struct thread *t = kmalloc(sizeof(struct thread));
    if (!t) return NULL;

    memset(t, 0, sizeof(struct thread));
    t->t_tid = tid;
    t->t_proc = p;
    t->t_state = 0; // READY

    // TODO: Multithreading
    p->p_threads = t;
    
    return t;
}

int proc_alloc_fd(struct proc *p, struct file *f) {
    if (!p || !f) return -EINVAL;

    for (int i = 0; i < MAX_FILES; i++) {
        if (p->p_fd_table[i] == NULL) {
            p->p_fd_table[i] = f;
            return i;
        }
    }

    return -EMFILE; // 빈자리가 없음!!
}