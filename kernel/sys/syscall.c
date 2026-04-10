#include <uapi/errno.h>
#include <kernel/printf.h>

typedef int64_t (*syscall_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

int64_t sys_test(uint64_t arg1) {
    dprintf("Syscall Test Success! Arg: 0x%lx\n", arg1);
    return 0xABCD;
}

static syscall_t syscall_table[] = {
    [1] = (syscall_t)sys_test,
    // TODO
};

int64_t do_syscall_handler(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    if (num >= sizeof(syscall_table)/sizeof(syscall_t) || !syscall_table[num]) {
        return -ENOSYS; 
    }

    return syscall_table[num](a1, a2, a3, a4, a5, 0);
}