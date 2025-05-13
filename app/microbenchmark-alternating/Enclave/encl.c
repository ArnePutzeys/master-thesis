#include "encl_t.h"
#include <sgx_trts.h>
#include "config_benchmark.h"
#define PAGE_SIZE 4096

// Internal buffer within enclave
// Allignment since otherwise code is on the same page, which we DO NOT want
// Do pagesize * 2 so its an even pagenumber (for debug purposes) (starts at page 30 usually)
static uint8_t internal_buffer[4096 * AMOUNT_OF_PAGES] __attribute__((aligned(PAGE_SIZE * 2)));

void ecall_access_alternating_pages()
{
    volatile uint8_t tmp;

    tmp = internal_buffer[0]; // Access Page

    return;
}

void *ecall_leak_internal_buffer_adrs(void)
{
    return (void *)internal_buffer;
}