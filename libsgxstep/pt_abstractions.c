#include "pt_abstractions.h"
#include "pt.h"
#include "debug.h"
#include <sys/mman.h>
#include "enclave.h"

#define MAX_PAGES 5000
static uint64_t *pte_cache[MAX_PAGES] = {NULL}; // Not the most optimal datastructure, TODO: Change to map

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

static inline void *pagenum_to_virt(size_t page)
{
    return (page << PT_SHIFT) + get_enclave_base();
}

static inline size_t virt_to_pagenum(void *virt)
{
    return ((size_t)virt - (size_t)get_enclave_base()) >> PT_SHIFT;
}

uint64_t *get_pte_by_page(size_t page)
{
    if (page >= MAX_PAGES)
    {
        return NULL; // Out of bounds
    }

    if (!pte_cache[page])
    {
        void *virt_addr = pagenum_to_virt(page);
        pte_cache[page] = remap_page_table_level(virt_addr, PTE);
    }

    return pte_cache[page];
}

uint64_t *get_pte_by_address(void *virt_addr)
{
    size_t page_num = virt_to_pagenum(virt_addr);

    if (page_num >= MAX_PAGES)
    {
        return NULL; // Out of bounds
    }

    if (!pte_cache[page_num])
    {
        pte_cache[page_num] = remap_page_table_level(virt_addr, PTE);
    }

    return pte_cache[page_num];
}

int pte_revoke_pages(size_t page, size_t num_pages)
{
    for (size_t i = 0; i < num_pages; i++)
    {
        uint64_t *pte = get_pte_by_page(page + i);
        if (!pte)
        {
            return -1; // Error retrieving PTE
        }
        pte_removeperms(pte);
    }
    return 0; // Success
}

int pte_restore_pages(size_t page, size_t num_pages)
{
    for (size_t i = 0; i < num_pages; i++)
    {
        uint64_t *pte = get_pte_by_page(page + i);
        if (!pte)
        {
            return -1; // Error retrieving PTE
        }
        pte_restoreperms(pte);
    }
    return 0; // Success
}