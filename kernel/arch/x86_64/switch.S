.intel_syntax noprefix

.set T_CONTEXT_OFF,   24
.set T_ARCH_DATA_OFF, 32

.section .text
.global context_switch

context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rax, 0xFFFFFFFF
    mov rdx, 0xFFFFFFFF
    mov rcx, [rdi + T_ARCH_DATA_OFF] # rcx = prev->t_arch_data
    xsaveopt [rcx]

    # Context Switch
    mov [rdi + T_CONTEXT_OFF], rsp
    mov rsp, [rsi + T_CONTEXT_OFF]

    mov rax, 0xFFFFFFFF
    mov rdx, 0xFFFFFFFF
    mov rcx, [rsi + T_ARCH_DATA_OFF] # rcx = next->t_arch_data
    xrstor [rcx]

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret