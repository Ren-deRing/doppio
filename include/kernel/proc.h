#pragma once

#include <uapi/types.h>

#include <kernel/fs/file.h>
#include <kernel/lock.h>

#define KSTACK_SIZE 4096
#define MAX_FILES 32

typedef enum {
    THREAD_EMBRYO,  // what?
    THREAD_READY,   // ready to work
    THREAD_RUNNING, // 
    THREAD_SLEEP,   // 
    THREAD_ZOMBIE   // holy god call init
} thread_state_t;

struct proc;
struct vnode;

// TCB
struct thread {
    tid_t           t_tid;
    struct proc    *t_proc;     /* 소속 프로세스 */
    
    void           *t_kstack;   /* 커널 스택 바닥 */
    void           *t_context;  /* 레지스터 컨텍스트 */
    void           *t_arch_data;/* 알아서 써라 */
    
    int             t_state;    /* 스레드 상태 */
    
    struct thread  *t_next;     /* 스레드 리스트 */
    struct thread  *t_sched_next;
};

// PCB
struct proc {
    pid_t           p_pid;
    spinlock_t      p_lock;

    struct file    *p_fd_table[MAX_FILES];
    struct vnode   *p_cwd;
    
    struct thread  *p_threads;
};

extern struct thread *curthread;

#define curproc (curthread ? curthread->t_proc : NULL)

struct proc* proc_create(pid_t pid);
struct thread* thread_create(struct proc *p, tid_t tid, void (*entry)(void));

int proc_alloc_fd(struct proc *p, struct file *f);