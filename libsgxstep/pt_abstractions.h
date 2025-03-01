#ifndef SGX_STEP_PT_ABSTRACTIONS_H
#define SGX_STEP_PT_ABSTRACTIONS_H

#include <stdint.h>
#include <stddef.h>

// Removes the permissions of a Page Table Entry
// Concretely: Unsets the User bit.
// Returns 1 if successful, -1 otherwise
int pte_removeperms(uint64_t *pte_pointer);

// Restores the permissions of a Page Table Entry
// Concretely: Sets the User bit.
// Returns 1 if successful, -1 otherwise
int pte_restoreperms(uint64_t *pte_pointer);

int pte_revoke_pages(size_t page, size_t num_pages);

int pte_restore_pages(size_t page, size_t num_pages);

uint64_t *get_pte_by_page(size_t page);

uint64_t *get_pte_by_address(void *virt_addr);

#endif // SGX_STEP_PT_ABSTRACTIONS_H