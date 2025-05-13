/* utility headers */
#include "libsgxstep/debug.h"
#include "libsgxstep/pf.h"
#include <sys/mman.h>
#include "libsgxstep/pt.h"
#include "libsgxstep/enclave.h"
#include "libsgxstep/pt_abstractions.h"
#include "libsgxstep/sched.h"
#include "libsgxstep/pf_abstractions.h"
#include "libsgxstep/cpu.h"
/*
Goal of the benchmark
1) Pagefault Gets triggered on page 30
2) rdtsc in fault handler
3) Revoke access to x sequential pages, with x a parameter
4) Restore access to page 30
5) rdtsc in fault handler

*/

/* Benchmark Setup */
#include "Enclave/config_benchmark.h"

/* SGX untrusted runtime */
#include <sgx_urts.h>
#include "Enclave/encl_u.h"

int benchmark_iteration = 0;
uint64_t begin;
uint64_t delta;

sgx_enclave_id_t create_enclave(void)
{
    sgx_launch_token_t token = {0};
    int updated = 0;
    sgx_enclave_id_t eid = -1;

#if DEBUG
    info_event("Creating enclave...");
#endif
    SGX_ASSERT(sgx_create_enclave("./Enclave/encl.so", /*debug=*/1,
                                  &token, &updated, &eid, NULL));

    return eid;
}

void fault_handler(size_t pagenum)
{
#if DEBUG
    info("Page fault on page %d:", pagenum);
#endif

    if (pagenum == BASE)
    {
        begin = rdtsc_begin();

#if DEBUG
        info("Revoking %d sequential pages starting at page %zu", benchmark_iteration, pagenum + 1);

#endif
#if USE_MPROTECT
        revoke_pages(pagenum + 1, benchmark_iteration);

#else
        pte_revoke_pages(pagenum + 1, benchmark_iteration);

#endif

        uint64_t end = rdtsc_end();
        delta = end - begin;
    }

// Restore page
#if USE_MPROTECT
    restore_pages(pagenum, 1);
#else
    pte_restore_pages(pagenum, 1);
#endif
}

int main(int argc, char **argv)
{
    // -----------------------------------------------------------
    // SETUP
    // -----------------------------------------------------------
    mlockall(MCL_FUTURE); // So that pages dont get unmapped

    sgx_enclave_id_t eid = create_enclave();
#if DEBUG
    info("Setup");
#endif
    ASSERT(!claim_cpu(VICTIM_CPU));
    ASSERT(!prepare_system_for_benchmark(PSTATE_PCT));
    // print_system_settings();

    register_fault_handler(fault_handler);

    void *buff_addrs;
    SGX_ASSERT(ecall_leak_internal_buffer_adrs(eid, &buff_addrs));
#if DEBUG
    info("Buffer addrs: %p", buff_addrs);

    info_event("Dry run: Calling enclave..");
#endif
    uint64_t irrelevant_output[N];
    SGX_ASSERT(ecall_access_sequential_pages(eid));

    // Dry run: Init state for PTE / mprotect
#if USE_MPROTECT
    revoke_pages(virt_to_pagenum(buff_addrs), AMOUNT_OF_PAGES);
    restore_pages(virt_to_pagenum(buff_addrs), AMOUNT_OF_PAGES);
#else
    pte_revoke_pages(virt_to_pagenum(buff_addrs), AMOUNT_OF_PAGES);
    pte_restore_pages(virt_to_pagenum(buff_addrs), AMOUNT_OF_PAGES);
#endif

// Do this as well otherwise first result has a way way bigger timing (2x)
#if USE_MPROTECT
    revoke_pages(virt_to_pagenum(buff_addrs), 1);
#else
    pte_revoke_pages(virt_to_pagenum(buff_addrs), 1);
#endif
    SGX_ASSERT(ecall_access_sequential_pages(eid));

    // -----------------------------------------------------------
    // Actual Benchmark
    // -----------------------------------------------------------
    uint64_t output[N];

    for (int i = 0; i <= N; i++)
    {
#if DEBUG
        info_event("Revoking First Page, iteration %d:", benchmark_iteration);
#endif

        // Restore access to the pages again
#if DEBUG
        info("Restoring %d sequential pages starting at page %zu", benchmark_iteration, virt_to_pagenum(buff_addrs) + 1);
#endif
#if USE_MPROTECT
        restore_pages(virt_to_pagenum(buff_addrs) + 1, benchmark_iteration);
#else
        pte_restore_pages(virt_to_pagenum(buff_addrs) + 1, benchmark_iteration);

#endif

        // Revoke the target page
#if USE_MPROTECT
        revoke_pages(virt_to_pagenum(buff_addrs), 1);
#else
        pte_revoke_pages(virt_to_pagenum(buff_addrs), 1);
#endif
        SGX_ASSERT(ecall_access_sequential_pages(eid));
        output[i] = delta;
        benchmark_iteration += 1;
    }

    for (int i = 0; i < N; i++)
    {
        printf("Time for iteration %d: %llu cycles.\n", i, output[i]);
    }
#if DEBUG
    info_event("destroying SGX enclave");
#endif
    SGX_ASSERT(sgx_destroy_enclave(eid));

    return 0;
}
