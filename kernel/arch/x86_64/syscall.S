.code64
.section .text

.global syscall_entry
.extern do_syscall_handler
.extern restore_and_exit

.set CPU_ARCH_DATA_OFFSET, 48   # struct cpu -> arch_cpu_data
.set AD_KERNEL_STACK,      0    # x86_64_cpu_data -> kernel_stack
.set AD_USER_RSP,          8    # x86_64_cpu_data -> user_rsp

syscall_entry:
    swapgs
    movq %gs:CPU_ARCH_DATA_OFFSET, %r11
    movq %rsp, AD_USER_RSP(%r11)
    movq AD_KERNEL_STACK(%r11), %rsp

    pushq $0x1B
    pushq AD_USER_RSP(%r11)
    
    pushq %r11
    
    pushq $0x23
    pushq %rcx

    pushq $0                   # Error
    pushq $0                   # Vector

    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %r8, %r9        # a5
    movq %r10, %r8       # a4
    movq %rdx, %rcx      # a3
    movq %rsi, %rdx      # a2
    movq %rdi, %rsi      # a1
    movq %rax, %rdi      # syscall num

    call do_syscall_handler

    jmp restore_and_exit