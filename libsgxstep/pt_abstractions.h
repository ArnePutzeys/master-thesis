#ifndef SGX_STEP_PT_ABSTRACTIONS_H
#define SGX_STEP_PT_ABSTRACTIONS_H

#include <stdint.h>


// Removes the permissions of a Page Table Entry
// Concretely: Unsets the User bit.
void pte_removeperms(uint64_t *pte_pointer);


// Restores the permissions of a Page Table Entry
// Concretely: Sets the User bit.
void pte_restoreperms(uint64_t *pte_pointer);

#endif // SGX_STEP_PT_ABSTRACTIONS_H