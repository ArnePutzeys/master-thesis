#include "pf.h"
#include "pt_abstractions.h"
#include "idt.h"
#include "cpu.h"
#include "sched.h"

fault_handler_t __fault_handler_cb = NULL;

void callback_wrapper_func(void)
{
    size_t pagenum = virt_to_pagenum((void *)__pf_irq_faultaddr);
    __fault_handler_cb(pagenum);
}

void setup_IDT_entry(void)
{
    // For stability purposes, still need to test without
    ASSERT(!claim_cpu(VICTIM_CPU));
    ASSERT(!prepare_system_for_benchmark(PSTATE_PCT));

    idt_t idt = {0};
    map_idt(&idt);
    void *original_gate_ptr = (void *)gate_offset(gate_ptr((&idt)->base, 14));
    __pf_irq_original_handler_addr = (uint64_t)original_gate_ptr;
    install_kernel_irq_handler(&idt, __pf_irq_handler, 14);
}

void register_fault_handler_IDT(fault_handler_t cb)
{
    setup_IDT_entry();
    __fault_handler_cb = cb;
    register_aep_cb(callback_wrapper_func);
}
