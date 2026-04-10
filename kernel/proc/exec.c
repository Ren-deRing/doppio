#include <kernel/exec.h>
#include <kernel/mmu.h>
#include <kernel/kmem.h>
#include <kernel/proc.h>
#include <kernel/cpu.h>
#include <uapi/elf.h>
#include <uapi/errno.h>
#include <string.h>

int proc_exec(struct proc *p, void *elf_data) {
    uintptr_t entry_point = 0;
    
    page_table_t *new_map = load_elf(elf_data, &entry_point);
    if (!new_map) {
        return -ENOEXEC;
    }

    uintptr_t stack_top = USER_STACK_TOP; 
    uintptr_t stack_bottom = stack_top - USER_STACK_SIZE;
    
    for (uintptr_t curr = stack_bottom; curr < stack_top; curr += PAGE_SIZE) {
        page_t *pg = page_alloc(0);
        if (!pg) {
            // 에이설마실패하겠나
            return -ENOMEM;
        }
        uintptr_t paddr = page_to_phys(pg);
        memset(p2v(paddr), 0, PAGE_SIZE);
        
        mmu_map(new_map, curr, paddr, MMU_FLAGS_USER | MMU_FLAGS_WRITE);
    }

    page_table_t *old_map = p->p_vm_map;
    p->p_vm_map = new_map;
    
    p->p_entry = entry_point;
    p->p_stack_top = stack_top;

    struct thread *cur = this_core->current; 
    if (cur && cur->t_proc == p) {
        cur->t_user_stack_top = stack_top;
    }

    if (old_map && old_map != mmu_get_kernel_map()) {
        mmu_destroy_map(old_map);
    }

    return 0;
}