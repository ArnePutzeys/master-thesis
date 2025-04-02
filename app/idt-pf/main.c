/*
 *  This file is part of the SGX-Step enclave execution control framework.
 *
 *  Copyright (C) 2017 Jo Van Bulck <jo.vanbulck@cs.kuleuven.be>,
 *                     Raoul Strackx <raoul.strackx@cs.kuleuven.be>
 *
 *  SGX-Step is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SGX-Step is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SGX-Step. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sgx_urts.h>
#include <signal.h>
#include <sys/mman.h>

#include "Enclave/encl_u.h"
#include "libsgxstep/debug.h"
#include "libsgxstep/elf_parser.h"
#include "libsgxstep/enclave.h"
#include "libsgxstep/pt.h"
#include "libsgxstep/idt.h"
#include "libsgxstep/gdt.h"
#include "libsgxstep/apic.h"
#include "libsgxstep/cpu.h"
#include "libsgxstep/sched.h"
#include "libsgxstep/config.h"
#include "libsgxstep/pt_abstractions.h"

#include "libsgxstep/pf_abstractions.h"

#define DBG_ENCL 1
#define USE_SIGNAL 0

void *data_pt = NULL, *data_page = NULL, *code_pt = NULL;
int fault_fired = 0, aep_fired = 0;
sgx_enclave_id_t eid = 0;

#if USE_SIGNAL

void fault_handler(int signo, siginfo_t *si, void *ctx)
{
    size_t pagenum = virt_to_pagenum(si->si_addr);
    pte_restore_pages(pagenum, 1);
    // restore_pages(pagenum, 1); // mprotect
}

void attacker_config_page_table(void)
{
    struct sigaction act, old_act;

    data_pt = get_symbol_offset("array") + get_enclave_base();
    data_page = (void *)((uintptr_t)data_pt & ~PFN_MASK);

    pte_revoke_pages(virt_to_pagenum(data_page), 1);
    // revoke_pages(virt_to_pagenum(data_page), 1); // mprotect

    /* Specify #PF handler with signinfo arguments */
    memset(&act, 0, sizeof(sigaction));
    act.sa_sigaction = fault_handler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    /* Block all signals while the signal is being handled */
    sigfillset(&act.sa_mask);
    ASSERT(!sigaction(SIGSEGV, &act, &old_act));
}

#else

void fault_handler_IDT(size_t pagenum)
{
    pte_restore_pages(pagenum, 1);
}

void attacker_config_page_table_IDT(void)
{

    data_pt = get_symbol_offset("array") + get_enclave_base();
    data_page = (void *)((uintptr_t)data_pt & ~PFN_MASK);

    pte_revoke_pages(virt_to_pagenum(data_page), 1);

    register_fault_handler_IDT(fault_handler_IDT);
}

#endif

int main(int argc, char **argv)
{
    ASSERT(!claim_cpu(VICTIM_CPU));
    ASSERT(!prepare_system_for_benchmark(PSTATE_PCT));

    //  info_event("Creating enclave...");
    sgx_launch_token_t token = {0};
    int retval = 0, updated = 0;
    char old = 0x00, new = 0xbb;

    SGX_ASSERT(sgx_create_enclave("./Enclave/encl.so", /*debug=*/DBG_ENCL,
                                  &token, &updated, &eid, NULL));

    // info("Dry run to allocate pages");
    SGX_ASSERT(enclave_dummy_call(eid, &retval));
    SGX_ASSERT(page_aligned_func(eid));

    register_symbols("./Enclave/encl.so");

#if USE_SIGNAL
    attacker_config_page_table();
#else
    attacker_config_page_table_IDT();
#endif

    // info_event("calling enclave data page fault..");
    uint64_t begin = rdtsc_begin();
    SGX_ASSERT(enclave_dummy_call(eid, &retval));
    uint64_t end = rdtsc_end();

    info("Total attack took %d cycles", end - begin);

    // info("all is well; exiting..");
    SGX_ASSERT(sgx_destroy_enclave(eid));
    return 0;
}
