#include "encl_t.h"
#include <sgx_trts.h>
#include "config_benchmark.h"
#define PAGE_SIZE 4096

// Internal buffer within enclave
// Allignment since otherwise code is on the same page, which we DO NOT want
// Do pagesize * 2 so its an even pagenumber (for debug purposes) (starts at page 30 usually)
static uint8_t internal_buffer[4096 * 1] __attribute__((aligned(PAGE_SIZE * 2)));

uint64_t rdtsc_begin(void)
{
    uint64_t begin;
    uint32_t a, d;

    asm volatile(
        "mfence\n\t"
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "mfence\n\t"
        : "=r"(d), "=r"(a)
        :
        : "%eax", "%ebx", "%ecx", "%edx");

    begin = ((uint64_t)d << 32) | a;
    return begin;
}

uint64_t rdtsc_end(void)
{
    uint64_t end;
    uint32_t a, d;

    asm volatile(
        "mfence\n\t"
        "RDTSCP\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "mfence\n\t"
        : "=r"(d), "=r"(a)
        :
        : "%eax", "%ebx", "%ecx", "%edx");

    end = ((uint64_t)d << 32) | a;
    return end;
}

uint64_t ecall_access_page(void)
{
    volatile uint8_t tmp;

    uint64_t begin = rdtsc_begin();
    tmp = internal_buffer[0]; // Access Page

    return begin;
}

void *ecall_leak_internal_buffer_adrs(void)
{
    return (void *)internal_buffer;
}