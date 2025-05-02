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
1) rdtsc
2) Pagefault Gets triggered
3) Control gets given to the fault handler -> rdtsc
4) Restore access to page
*/

/* Benchmark Setup */
#include "Enclave/config_benchmark.h"

/* SGX untrusted runtime */
#include <sgx_urts.h>
#include "Enclave/encl_u.h"

uint64_t end = 0;

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
    end = rdtsc_end();
#if DEBUG
    info("Page fault on page %d:", pagenum);
#endif

    // Restore page
    pte_restore_pages(pagenum, 1);
}

int main(int argc, char **argv)
{
    // -----------------------------------------------------------
    // SETUP
    // -----------------------------------------------------------
    mlockall(MCL_FUTURE); // So that pages dont get unmapped, not needed for pte revocation but do it anyway

    sgx_enclave_id_t eid = create_enclave();
#if DEBUG
    info("Setup");
#endif
    ASSERT(!claim_cpu(VICTIM_CPU));
    ASSERT(!prepare_system_for_benchmark(PSTATE_PCT));

#if USE_CUSTOM_IDT
    register_fault_handler_IDT(fault_handler);
#else
    register_fault_handler(fault_handler);
#endif
    void *buff_addrs;
    SGX_ASSERT(ecall_leak_internal_buffer_adrs(eid, &buff_addrs));
#if DEBUG
    info("Buffer addrs: %p", buff_addrs);

    info_event("Dry run: Calling enclave..");
#endif
    uint64_t irrelevant_output[N];
    // Dry run
    pte_revoke_pages(virt_to_pagenum(buff_addrs), 1);
    SGX_ASSERT(ecall_access_page(eid, &irrelevant_output[1]));

    // -----------------------------------------------------------
    // Actual Benchmark
    // -----------------------------------------------------------
    uint64_t output[N];

    for (int i = 0; i < N; i++)
    {
#if DEBUG
        info_event("Revoking Page, iteration %d:", benchmark_iteration);
#endif

        pte_revoke_pages(virt_to_pagenum(buff_addrs), 1);
        SGX_ASSERT(ecall_access_page(eid, &output[i]));
        output[i] = end - output[i];
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
