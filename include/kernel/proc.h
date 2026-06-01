#pragma once

#include <uapi/types.h>
#include <stdbool.h>

#include <kernel/mmu.h>
#include <kernel/fs/file.h>
#include <kernel/lock.h>

#define MAX_FILES 32

#define THREAD_FLAG_USER   (1 << 0)
#define THREAD_FLAG_KERNEL (1 << 1)

extern pid_t next_pid;
extern tid_t next_tid;

typedef enum {
    THREAD_EMBRYO,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEP,
    THREAD_WAITING,
    THREAD_ZOMBIE
} thread_state_t;

typedef enum {
    PROC_RUNNING,
    PROC_ZOMBIE,
    PROC_STOPPED,
} proc_state_t;

struct proc;
struct vnode;
struct arch_proc;

struct trapframe {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;

    // uint64_t err_code;
};

// TCB
struct thread {
    tid_t            t_tid;
    struct proc     *t_proc;

    void            *t_kstack;
    void            *t_context;
    void            *t_arch_data;
    struct trapframe *t_trapframe; 

    int              t_state;

    struct thread   *t_sched_next;
    bool             t_need_resched;
    uint32_t         t_ticks;

    uint64_t         t_sleep_until;
    struct list_node t_wait_node;
    spinlock_t      *t_lock_to_release;

    uintptr_t        t_fs_base;

    uint64_t        t_sig_pending;
    uint64_t        t_sig_mask;

    void            (*t_entry)(void *);
    void            *t_arg;
    int              t_flags;

    uintptr_t        t_user_stack_top;

    int              t_errno;
};

// PCB
struct proc {
    pid_t           p_pid;
    uid_t           p_uid, p_euid;
    gid_t           p_gid, p_egid;
    proc_state_t    p_state;
    spinlock_t      p_lock;

    char            p_name[16];
    mode_t          p_umask;

    struct file    *p_fd_table[MAX_FILES];
    struct vnode   *p_cwd;

    page_table_t   *p_vm_map;
    uintptr_t       p_entry;
    uintptr_t       p_stack_top;
    uintptr_t       p_brk;
    uintptr_t       p_mmap_base;

    struct arch_proc *p_arch;

    struct thread    *p_threads;

    struct proc      *p_parent;
    struct list_node  p_child_link;
    struct list_node  p_children;
};



struct proc* proc_create(pid_t pid);
struct thread* thread_create(struct proc *p, tid_t tid, void (*entry)(void *), void *arg);

int proc_alloc_fd(struct proc *p, struct file *f);