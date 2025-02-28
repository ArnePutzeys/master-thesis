#include "pt_abstractions.h"
#include "pt.h"
#include "debug.h"

void pte_removeperms(uint64_t *pte_pointer)
{
    ASSERT(PRESENT(*pte_pointer));
    *pte_pointer = MARK_SUPERVISOR(*pte_pointer);
}

void pte_restoreperms(uint64_t *pte_pointer)
{
    ASSERT(PRESENT(*pte_pointer));
    *pte_pointer = MARK_USER(*pte_pointer);
}