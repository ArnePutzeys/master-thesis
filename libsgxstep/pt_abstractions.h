#ifndef SGX_STEP_PT_ABSTRACTIONS_H
#define SGX_STEP_PT_ABSTRACTIONS_H

#include <stdint.h>
#include <stddef.h>

int pte_revoke_pages(size_t page, size_t num_pages);

int pte_restore_pages(size_t page, size_t num_pages);

uint64_t *get_pte_by_page(size_t page);

uint64_t *get_pte_by_address(void *virt_addr);

#endif // SGX_STEP_PT_ABSTRACTIONS_H