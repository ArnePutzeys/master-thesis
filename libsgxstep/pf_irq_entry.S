.section isr_section,"awx",@progbits
.align 0x1000
.global __pf_irq_original_handler_addr, __pf_irq_debugreg
__pf_irq_original_handler_addr:
    .quad 0x0    
__pf_irq_debugreg:
    .quad 0x0

.section isr_section,"awx",@progbits
.align 0x1000
.global __pf_irq_handler
__pf_irq_handler:

    # Save registers
    push %rax
    push %rbx
    push %rdx
    pushfq

    # Load the error code
    mov 32(%rsp), %rax

    # Check if the SGX bit (Bit 15) is set
    # need to save the rflags register as well
    bt $15, %rax
    jc 1f
    # test $0x8000, %rax # Alternative way of checking bit 15
    # jnz 1f

    # SGX bit isn't set
    # Restore registers
    popfq
    pop %rdx
    pop %rbx
    pop %rax

    # Call Original handler
    jmp *__pf_irq_original_handler_addr(%rip)


1:
    # Just return without calling a handler, we register the handler 
    # in the aep callback, just need to signal a pagenumber for example

    # SGX bit is set
    # Restore registers
    movq $1, __pf_irq_debugreg(%rip)
    popfq
    pop %rdx
    pop %rbx
    pop %rax
    add $8, %rsp  # Remove error code from the stack
    iretq


