.section isr_section,"awx",@progbits
.align 0x1000
.global __pf_irq_original_handler_addr, __pf_irq_debugreg
__pf_irq_original_handler_addr:
    .quad 0x0    
__pf_irq_debugreg:
    .quad 0x0
__pf_irq_rax:
    .quad 0x0
__pf_irq_rbx:
    .quad 0x0
__pf_irq_rdx:
    .quad 0x0
__pf_irq_rcx:
    .quad 0x0
__pf_irq_rip:
    .quad 0x0
__pf_irq_rflags:
    .quad 0x0

.section isr_section,"awx",@progbits
.align 0x1000
.global __pf_irq_handler
__pf_irq_handler:

    # Save registers
    mov %rax, __pf_irq_rax(%rip)
    mov %rbx, __pf_irq_rbx(%rip)
    mov %rdx, __pf_irq_rdx(%rip)

    # Save rflags
    
    pushfq
    pop %rax
    mov %rax, __pf_irq_rflags(%rip)

    # Load the error code
    mov 8(%rsp), %rax

    # Check if the SGX bit (Bit 15) is set
    # need to save the rflags register as well
    test $0x8000, %rax
    jnz 1f

    mov $0, __pf_irq_debugreg(%rip)

    # Restore rflags
    mov __pf_irq_rflags(%rip), %rax
    push %rax
    popfq

    # Restore registers
    mov __pf_irq_rax(%rip), %rax
    mov __pf_irq_rbx(%rip), %rbx
    mov __pf_irq_rdx(%rip), %rdx

    # Call Original handler
    jmp *__pf_irq_original_handler_addr(%rip)


1:
    mov $1, __pf_irq_debugreg(%rip)
    # Restore rflags
    mov __pf_irq_rflags(%rip), %rax
    push %rax
    popfq

    # Restore registers
    mov __pf_irq_rax(%rip), %rax
    mov __pf_irq_rbx(%rip), %rbx
    mov __pf_irq_rdx(%rip), %rdx

    # Call Original handler once again but replace with custom one
    # later
    jmp *__pf_irq_original_handler_addr(%rip)


