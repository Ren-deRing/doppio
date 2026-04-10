#pragma once

#include <uapi/types.h>
#include <stdbool.h>

#include <kernel/mmu.h>
#include <kernel/fs/file.h>
#include <kernel/lock.h>

#define KSTACK_SIZE 4096
#define MAX_FILES 32

#define THREAD_FLAG_USER   (1 << 0)
#define THREAD_FLAG_KERNEL (1 << 1)

typedef enum {
    THREAD_EMBRYO,  // what?
    THREAD_READY,   // ready to work
    THREAD_RUNNING, // 
    THREAD_SLEEP,   //
    THREAD_WAITING, 
    THREAD_ZOMBIE   // holy god call init
} thread_state_t;

struct proc;
struct vnode;
struct arch_proc;

// TCB
struct thread {
    tid_t            t_tid;
    struct proc     *t_proc;
    
    void            *t_kstack;   /* 커널 스택 바닥 */
    void            *t_context;  /* 레지스터 컨텍스트 */
    void            *t_arch_data;/* 알아서 써라 */
    
    int              t_state;
    
    struct thread   *t_next;
    struct thread   *t_sched_next;

    // ---------------------

    bool             t_need_resched;
    uint32_t         t_ticks;

    uint64_t         t_sleep_until;
    struct list_node t_wait_node;
    spinlock_t      *t_lock_to_release;

    void            *t_arg;
    int              t_flags;

    uintptr_t        t_user_stack_top;
};

// PCB
struct proc {
    pid_t           p_pid;
    spinlock_t      p_lock;

    struct file    *p_fd_table[MAX_FILES];
    struct vnode   *p_cwd;
    
    page_table_t   *p_vm_map;
    uintptr_t       p_entry;
    uintptr_t       p_stack_top;

    struct arch_proc *p_arch;

    struct thread  *p_threads;
    struct proc    *p_parent;
};

extern struct thread *curthread;

#define curproc (curthread ? curthread->t_proc : NULL)

struct proc* proc_create(pid_t pid);
struct thread* thread_create(struct proc *p, tid_t tid, void (*entry)(void), void *arg);

int proc_alloc_fd(struct proc *p, struct file *f);