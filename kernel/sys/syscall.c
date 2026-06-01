#include <uapi/errno.h>
#include <kernel/printf.h>
#include <kernel/mmu.h>
#include <kernel/cpu.h>
#include <kernel/proc.h>
#include <kernel/sched.h>
#include <kernel/kmem.h>
#include <kernel/exec.h>

#include <kernel/fs/vfs.h>
#include <kernel/fs/vnode.h>

#include <uapi/sys/stat.h>
#include <uapi/fcntl.h>

#include <string.h>

#define USER_ADDR_LIMIT  0x0000800000000000UL

typedef int64_t (*syscall_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static inline bool is_user_address_range(const void *addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;

    if (end < start) return false;

    if (end > USER_ADDR_LIMIT) return false;

    return true;
}

int copy_to_user(void *user_dest, const void *src, size_t n) {
    page_table_t *map = curthread->t_proc->p_vm_map;
    uintptr_t dest = (uintptr_t)user_dest;
    uint8_t *s = (uint8_t *)src;
    size_t copied = 0;

    while (copied < n) {
        uintptr_t paddr = mmu_translate(map, dest + copied);
        if (!paddr) return -1;

        size_t page_offset = (dest + copied) & 0xFFF;
        size_t to_copy = 4096 - page_offset;
        if (to_copy > (n - copied)) to_copy = n - copied;

        memcpy((void *)p2v(paddr), s + copied, to_copy);
        copied += to_copy;
    }
    return 0;
}

int copy_from_user(void *dest, const void *user_src, size_t n) {
    page_table_t *map = curthread->t_proc->p_vm_map;
    uintptr_t src = (uintptr_t)user_src;
    uint8_t *d = (uint8_t *)dest;
    size_t copied = 0;

    while (copied < n) {
        uintptr_t paddr = mmu_translate(map, src + copied);
        if (!paddr) return -1;

        size_t page_offset = (src + copied) & 0xFFF;
        size_t to_copy = 4096 - page_offset;
        if (to_copy > (n - copied)) to_copy = n - copied;

        memcpy(d + copied, (void *)p2v(paddr), to_copy);
        copied += to_copy;
    }
    return 0;
}

int64_t sys_write(int fd, const void *user_buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILES) return -EBADF;
    if (count == 0) return 0;

    if (!is_user_address_range(user_buf, count)) return -EFAULT;

    if (fd == 1 || fd == 2) {
        char kbuf[256];
        size_t total = 0;

        while (total < count) {
            size_t to_copy = count - total;
            if (to_copy > 255) to_copy = 255;

            if (copy_from_user(kbuf, (const char *)user_buf + total, to_copy) < 0) {
                return -EFAULT;
            }

            for (size_t i = 0; i < to_copy; i++) {
                kputc(kbuf[i]);
            }
            total += to_copy;
        }
        return (int64_t)count;
    }

    return (int64_t)vfs_write(fd, user_buf, count);
}

int64_t sys_writev(int fd, const struct iovec *user_iov, int iovcnt) {
    if (fd < 0 || fd >= MAX_FILES) return -EBADF;
    if (iovcnt <= 0 || iovcnt > 1024) return -EINVAL;

    struct { void *base; size_t len; } kiov[iovcnt];
    if (copy_from_user(kiov, user_iov, iovcnt * sizeof(kiov[0])) < 0)
        return -EFAULT;

    int64_t total = 0;
    for (int i = 0; i < iovcnt; i++) {
        if (kiov[i].len == 0) continue;
        int64_t r = sys_write(fd, kiov[i].base, kiov[i].len);
        if (r < 0) return total > 0 ? total : r;
        total += r;
    }
    return total;
}

int64_t sys_exit(int64_t status) {
    cpu_status_t flags = arch_irq_save();

    curthread->t_state = THREAD_ZOMBIE; 
    curproc->p_state = PROC_ZOMBIE;
    curthread->t_need_resched = true;

    return 0; 
}

int64_t sys_mmap(uintptr_t addr, size_t length, int prot, int flags, int fd, int64_t offset) {
    if (length == 0) return -EINVAL;

    if (addr == 0) {
        addr = curproc->p_mmap_base;
        curproc->p_mmap_base += ALIGN_UP(length, PAGE_SIZE);
    }

    uintptr_t start = ALIGN_DOWN(addr, PAGE_SIZE);
    uintptr_t end = ALIGN_UP(addr + length, PAGE_SIZE);

    for (uintptr_t i = start; i < end; i += PAGE_SIZE) {
        page_t *pg = page_alloc(0);
        if (!pg) return -ENOMEM;

        uintptr_t paddr = page_to_phys(pg);
        memset(p2v(paddr), 0, PAGE_SIZE);

        uint32_t mmu_flags = MMU_FLAGS_USER;
        if (prot & 0x2) mmu_flags |= MMU_FLAGS_WRITE; // PROT_WRITE

        if (!mmu_map(curproc->p_vm_map, i, paddr, mmu_flags)) {
            page_free(pg, 0);
            return -ENOMEM;
        }
    }

    return addr;
}

int64_t sys_munmap(uintptr_t addr, size_t length) {
    if (length == 0) return -EINVAL;
    if (!is_user_address_range((void *)addr, length)) return -EINVAL;

    uintptr_t start = ALIGN_DOWN(addr, PAGE_SIZE);
    uintptr_t end = ALIGN_UP(addr + length, PAGE_SIZE);
    page_table_t *map = curproc->p_vm_map;

    for (uintptr_t i = start; i < end; i += PAGE_SIZE) {
        uintptr_t paddr = mmu_translate(map, i);
        
        if (paddr != 0) {
            page_t *pg = phys_to_page(paddr);
            mmu_unmap(map, i);

            if (pg && pg->ref_count == 0) {
                page_free(pg, 0); 
            }
        }
    }

    return 0;
}

int64_t sys_brk(uintptr_t brk) {
    uintptr_t old_brk = curproc->p_brk;
    if (brk == 0) {
        return old_brk;
    }

    if (brk > old_brk) {
        uintptr_t start = ALIGN_UP(old_brk, PAGE_SIZE);
        uintptr_t end = ALIGN_UP(brk, PAGE_SIZE);

        if (end > curproc->p_stack_top) {
            return old_brk;
        }

        for (uintptr_t i = start; i < end; i += PAGE_SIZE) {
            if (mmu_translate(curproc->p_vm_map, i) == 0) {
                page_t* pg = page_alloc(0);
                if (!pg) {
                    return old_brk;
                }
                uintptr_t paddr = page_to_phys(pg);
                memset(p2v(paddr), 0, PAGE_SIZE);
                if (!mmu_map(curproc->p_vm_map, i, paddr, MMU_FLAGS_USER | MMU_FLAGS_WRITE)) {
                    page_free(pg, 0);
                    return old_brk; 
                }
            }
        }
    } else if (brk < old_brk) {
    }

    curproc->p_brk = brk;
    return brk;
}

int64_t sys_open(const char *user_path, int flags, mode_t mode) {
    char kpath[256];
    if (copy_from_user(kpath, user_path, 256) < 0) return -EFAULT;
    kpath[255] = '\0';

    int fd_out = -1;
    int err = vfs_open(kpath, flags, mode, &fd_out);
    if (err < 0) return (int64_t)err;
    return (int64_t)fd_out;
}

int64_t sys_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES) return -EBADF;

    struct file *f = curproc->p_fd_table[fd];
    if (!f) return -EBADF;

    curproc->p_fd_table[fd] = NULL;

    file_close(f); 
    return 0;
}

int64_t sys_read(int fd, void *user_buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILES) return -EBADF;
    if (count == 0) return 0;
    if (!is_user_address_range(user_buf, count)) return -EFAULT;

    char kbuf[4096];
    size_t total = 0;

    while (total < count) {
        size_t to_copy = count - total;
        if (to_copy > sizeof(kbuf)) to_copy = sizeof(kbuf);

        int n = vfs_read(fd, kbuf, to_copy);
        if (n < 0) return (total == 0) ? n : (int64_t)total;
        if (n == 0) break;

        if (copy_to_user((char *)user_buf + total, kbuf, n) < 0)
            return -EFAULT;
        total += n;
    }
    return (int64_t)total;
}

int sys_fork(void) {
    struct proc *parent_p = curproc;
    struct thread *parent_t = curthread;

    struct proc *child_p = proc_create(next_pid++);
    if (!child_p) return -ENOMEM;

    child_p->p_parent = parent_p;
    child_p->p_uid = parent_p->p_uid;
    child_p->p_euid = parent_p->p_euid;
    child_p->p_gid = parent_p->p_gid;
    child_p->p_egid = parent_p->p_egid;
    child_p->p_umask = parent_p->p_umask;
    memcpy(child_p->p_name, parent_p->p_name, sizeof(parent_p->p_name));

    if (parent_p->p_cwd) {
        child_p->p_cwd = parent_p->p_cwd;
        vref(parent_p->p_cwd);
    }

    spin_lock(&parent_p->p_lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (parent_p->p_fd_table[i] != NULL) {
            child_p->p_fd_table[i] = parent_p->p_fd_table[i];
            
            child_p->p_fd_table[i]->f_refcnt++; 
        } else {
            child_p->p_fd_table[i] = NULL;
        }
    }
    spin_unlock(&parent_p->p_lock);

    child_p->p_vm_map = mmu_clone_map(parent_p->p_vm_map);
    if (!child_p->p_vm_map) {
        return -ENOMEM;
    }
    child_p->p_entry = parent_p->p_entry;
    child_p->p_stack_top = parent_p->p_stack_top;
    child_p->p_brk = parent_p->p_brk;
    child_p->p_mmap_base = parent_p->p_mmap_base;

    uintptr_t user_rsp = parent_t->t_trapframe->rsp;
    uintptr_t parent_paddr = mmu_translate(parent_p->p_vm_map, user_rsp);
    uintptr_t child_paddr = mmu_translate(child_p->p_vm_map, user_rsp);

    struct thread *child_t = kmalloc_aligned(sizeof(struct thread), 64);
    if (!child_t) {
        // TODO: 자원 해제
        return -ENOMEM;
    }
    memset(child_t, 0, sizeof(struct thread));

    void *child_stack = kmalloc_aligned(KSTACK_SIZE, KSTACK_SIZE);
    if (!child_stack) {
        // TODO: 자원 해제
        return -ENOMEM;
    }
    child_t->t_kstack = child_stack;

    int err = arch_thread_fork(child_t, parent_t);
    if (err < 0) {
        // 청소 로직
        return err;
    }

    child_t->t_tid = next_tid++;
    child_t->t_proc = child_p;
    child_t->t_state = THREAD_READY;
    child_t->t_flags = parent_t->t_flags;
    
    spin_lock(&parent_p->p_lock);
    list_add_tail(&child_p->p_child_link, &parent_p->p_children);
    spin_unlock(&parent_p->p_lock);

    sched_enqueue(child_t);
    return child_p->p_pid;
}

int64_t sys_mprotect(uintptr_t start, size_t len, int prot) {
    if (len == 0) return 0;
    if (!is_user_address_range((void *)start, len)) return -EINVAL;

    uintptr_t addr = ALIGN_DOWN(start, PAGE_SIZE);
    uintptr_t end = ALIGN_UP(start + len, PAGE_SIZE);
    page_table_t *map = curproc->p_vm_map;

    for (uintptr_t i = addr; i < end; i += PAGE_SIZE) {
        if (mmu_translate(map, i) == 0) {
            return -ENOMEM;
        }
    }

    for (uintptr_t i = addr; i < end; i += PAGE_SIZE) {
        mmu_protect_page(map, i, prot);
    }

    return 0;
}

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

int64_t sys_arch_prctl(int code, uintptr_t addr) {
    if (code == ARCH_SET_FS) {
        if (!is_user_address_range((void *)addr, sizeof(void *))) {
            return -EFAULT;
        }

        arch_cpu_set_fs_base(addr);

        curthread->t_fs_base = addr;

        return 0;
    }

    return -EINVAL;
}

int64_t sys_set_tid_address(int *tidptr) {
    if (tidptr) {
        if (!is_user_address_range(tidptr, sizeof(int))) return -EFAULT;
    }
    
    if (curthread->t_tid == 0) curthread->t_tid = 1; 
    return (int64_t)curthread->t_tid;
}

int64_t sys_execve(const char *user_path, char *const argv[], char *const envp[]) {
    char kpath[256];
    if (copy_from_user(kpath, user_path, 256) < 0) {
        return -EFAULT;
    }

    struct vnode *vn = NULL;
    int err = vfs_lookup(kpath, curproc->p_cwd, &vn);
    if (err < 0) {
        return err;
    }

    size_t allocated_size = 64 * 1024;
    void *elf_data = kmalloc(allocated_size);
    size_t file_size = 0;

    while (1) {
        if (file_size >= allocated_size) {
            size_t new_size = allocated_size * 2;
            void *new_buf = kmalloc(new_size);
            memcpy(new_buf, elf_data, file_size);
            kfree(elf_data);
            elf_data = new_buf;
            allocated_size = new_size;
        }
        size_t space_left = allocated_size - file_size;
        
        int n = vn->ops->read(vn, (void *)((uintptr_t)elf_data + file_size), space_left, file_size);
        
        if (n < 0) {
            kfree(elf_data);
            vput(vn);
            return n;
        }
        if (n == 0) {
            break;
        }

        file_size += n;
    }

    vput(vn); 

    uintptr_t entry_point = 0;
    uintptr_t brk = 0;
    uintptr_t phdr_vaddr = 0;
    uint64_t phnum = 0;
    page_table_t *new_map = load_elf(elf_data, &entry_point, &brk, &phdr_vaddr, &phnum);
    
    kfree(elf_data);
    
    if (!new_map) {
        dprintf("load_elf FAILED! Invalid ELF Magic or Format.\n");
        return -ENOEXEC; 
    }

    uintptr_t stack_top = USER_STACK_TOP;
    uintptr_t stack_bottom = USER_STACK_TOP - USER_STACK_SIZE;

    for (uintptr_t curr = stack_bottom; curr < stack_top; curr += PAGE_SIZE) {
        page_t *pg = page_alloc(0);
        if (!pg) {
            dprintf("Memory Allocation for User Stack FAILED.\n");
            mmu_destroy_map(new_map);
            return -ENOMEM;
        }
        uintptr_t paddr = page_to_phys(pg);
        memset(p2v(paddr), 0, PAGE_SIZE);
        mmu_map(new_map, curr, paddr, MMU_FLAGS_USER | MMU_FLAGS_WRITE);
    }

    uintptr_t final_user_rsp = setup_user_stack(new_map, USER_STACK_TOP, argv, envp, phdr_vaddr, phnum);
    if (final_user_rsp == 0 || (final_user_rsp % 16) != 0) {
        dprintf("setup_user_stack FAILED. Misaligned RSP: %p\n", final_user_rsp);
        mmu_destroy_map(new_map);
        return -EFAULT;
    }

    page_table_t *old_map = curproc->p_vm_map;
    curproc->p_vm_map = new_map;
    curproc->p_entry = entry_point;
    curproc->p_brk = ALIGN_UP(brk, PAGE_SIZE);
    curproc->p_stack_top = USER_STACK_TOP;

    arch_switch_mm(NULL, curproc);
    mmu_destroy_map(old_map); 

    arch_set_kernel_stack((uintptr_t)curthread->t_kstack + KSTACK_SIZE);
    arch_exec_setup_trapframe(curthread->t_trapframe, entry_point, final_user_rsp);

    curthread->t_trapframe->rax = 0;

    return curthread->t_trapframe->rax; 
}

int64_t sys_fstat(int fd, struct stat *user_statbuf) {
    if (fd < 0 || fd >= MAX_FILES) return -EBADF;
    if (!is_user_address_range(user_statbuf, sizeof(struct stat))) return -EFAULT;

    struct stat kst;
    memset(&kst, 0, sizeof(struct stat));

    if (fd == 0 || fd == 1 || fd == 2) {
        kst.st_mode = S_IFCHR | 0666; 
        kst.st_blksize = 1024;
        kst.st_blocks = 0;
        kst.st_size = 0;
    } else {
        struct file *f = curproc->p_fd_table[fd];
        if (!f || !f->f_vn) return -EBADF;

        kst.st_mode = S_IFREG | 0644;
        kst.st_blksize = 4096;
        kst.st_size = 0;
    }

    if (copy_to_user(user_statbuf, &kst, sizeof(struct stat)) < 0) {
        return -EFAULT;
    }

    return 0;
}

int64_t sys_newfstatat(int dirfd, const char *user_path, struct stat *user_statbuf, int flags) {
    if (!is_user_address_range(user_statbuf, sizeof(struct stat))) return -EFAULT;

    if (flags & AT_EMPTY_PATH) {
        return sys_fstat(dirfd, user_statbuf);
    }

    char kpath[256];
    if (copy_from_user(kpath, user_path, 256) < 0) return -EFAULT;

    if (strlen(kpath) == 0) {
        return sys_fstat(dirfd, user_statbuf);
    }

    struct vnode *vn = NULL;
    int err = vfs_lookup(kpath, curproc->p_cwd, &vn);
    if (err < 0) return err;

    struct stat kst;
    memset(&kst, 0, sizeof(struct stat));

    kst.st_mode = S_IFREG | 0644; 
    kst.st_blksize = 4096;
    kst.st_size = 0;

    vput(vn);

    if (copy_to_user(user_statbuf, &kst, sizeof(struct stat)) < 0) {
        return -EFAULT;
    }

    return 0;
}

int64_t sys_getpid(void) {
    if (curproc && curproc->p_pid > 0) {
        return curproc->p_pid;
    }
    return 1;
}

int64_t sys_ioctl(int fd, uint64_t request, void *arg) {
    if (request == 0x5413) {
        if (!is_user_address_range(arg, 8)) {
            return -EFAULT;
        }

        unsigned short fake_winsize[4] = { 25, 80, 0, 0 };

        if (copy_to_user(arg, fake_winsize, 8) < 0) {
            return -EFAULT;
        }

        return 0;
    }

    return -ENOTTY;
}

int64_t sys_getrlimit(int resource, void *user_rlim) {
    struct { uint64_t cur, max; } rl = { 8*1024*1024UL, (uint64_t)-1 };
    if (!is_user_address_range(user_rlim, sizeof(rl))) return -EFAULT;
    if (copy_to_user(user_rlim, &rl, sizeof(rl)) < 0) return -EFAULT;
    return 0;
}

int64_t sys_clock_gettime(int clk_id, void *user_tp) {
    struct { int64_t tv_sec; int64_t tv_nsec; } tp = {0, 0};
    if (!is_user_address_range(user_tp, sizeof(tp))) return -EFAULT;
    if (copy_to_user(user_tp, &tp, sizeof(tp)) < 0) return -EFAULT;
    return 0;
}

int64_t sys_rt_sigaction(int sig, const void *act, void *oact, size_t sigsetsize) {
    (void)sig; (void)act; (void)oact; (void)sigsetsize;
    return 0;
}

int64_t sys_rt_sigprocmask(int how, const void *set, void *oset, size_t sigsetsize) {
    (void)how; (void)set; (void)sigsetsize;
    if (oset && is_user_address_range(oset, sigsetsize)) {
        uint64_t zero = 0;
        copy_to_user(oset, &zero, sizeof(zero));
    }
    return 0;
}

static syscall_t syscall_table[] = {
    [0] = (syscall_t)sys_read,
    [1] = (syscall_t)sys_write,
    [2] = (syscall_t)sys_open,
    [3] = (syscall_t)sys_close,
    [5] = (syscall_t)sys_fstat,
    [9]  = (syscall_t)sys_mmap,
    [10] = (syscall_t)sys_mprotect,
    [11] = (syscall_t)sys_munmap,
    [12] = (syscall_t)sys_brk,
    [13] = (syscall_t)sys_rt_sigaction,
    [14] = (syscall_t)sys_rt_sigprocmask,
    [16] = (syscall_t)sys_ioctl,
    [20] = (syscall_t)sys_writev,
    [39] = (syscall_t)sys_getpid,
    [57] = (syscall_t)sys_fork,
    [59] = (syscall_t)sys_execve,
    [60] = (syscall_t)sys_exit,
    // [63]  = (syscall_t)sys_uname,
    [97]  = (syscall_t)sys_getrlimit,
    [158] = (syscall_t)sys_arch_prctl,
    [218] = (syscall_t)sys_set_tid_address,
    [228] = (syscall_t)sys_clock_gettime,
    [231] = (syscall_t)sys_exit, // sys_exit_group
    [262] = (syscall_t)sys_newfstatat,
};

int64_t do_syscall_handler(struct trapframe *tf) {
    curthread->t_trapframe = tf;

    uint64_t num = tf->rax;

    uint64_t a1 = tf->rdi;
    uint64_t a2 = tf->rsi;
    uint64_t a3 = tf->rdx;
    uint64_t a4 = tf->r10;
    uint64_t a5 = tf->r8;

    if (num >= sizeof(syscall_table)/sizeof(syscall_t) ||
        !syscall_table[num]) {
        tf->rax = -ENOSYS;
        return tf->rax;
    }

    int64_t ret =
        syscall_table[num](a1, a2, a3, a4, a5, 0);

    tf->rax = ret;

    return ret;
}