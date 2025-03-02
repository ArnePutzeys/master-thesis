#include "pt_abstractions.h"
#include "pt.h"
#include "debug.h"
#include <sys/mman.h>
#include "enclave.h"

#define MAX_PAGES 5000
static uint64_t *pte_cache[MAX_PAGES] = {NULL}; // Not the most optimal datastructure, TODO: Change to map

int pte_removeperms(uint64_t *pte_pointer)
{
    if (!PRESENT(*pte_pointer))
        return -1;

    *pte_pointer = MARK_SUPERVISOR(*pte_pointer);
    return 1;
}

int pte_restoreperms(uint64_t *pte_pointer)
{
    if (!PRESENT(*pte_pointer))
        return -1;

    *pte_pointer = MARK_USER(*pte_pointer);
    return 1;
}

static inline void *pagenum_to_virt(size_t page)
{
    // Both are equivalent:
    return (page << PT_SHIFT) + (size_t)get_enclave_base();
    // return (size_t)get_enclave_base() + page * PAGE_SIZE_4KiB;
}

static inline size_t virt_to_pagenum(void *virt)
{
    return ((size_t)virt - (size_t)get_enclave_base()) >> PT_SHIFT;
}

uint64_t *get_pte_by_page(size_t page)
{
    if (page >= MAX_PAGES)
    {
        info("Out of bounds");
        return NULL; // Out of bounds
    }

    if (!pte_cache[page])
    {
        info("PTE for page %d not in page cache yet, adding it", page);
        void *virt_addr = pagenum_to_virt(page);
        info("Page %d translated to virtual address %p. Enclave Base: %p", page, virt_addr, get_enclave_base());

        uint8_t buffer[4096];
        info("Reading contents of page with edbgrd");

        edbgrd(virt_addr, buffer, 4096);

        // Print buffer in hex format
        printf("Page contents:\n");
        for (int j = 0; j < 128; j++)
        {
            if (j % 16 == 0)
                printf("\n");
            printf("%02X ", buffer[j]);
        }
        printf("\n");

        void *pte = remap_page_table_level(virt_addr, PTE);
        info("PTE: %p for page %d", pte, page);

        print_page_table(virt_addr);
        // info("Virt Addr: %p, PTE: %p", virt_addr, pte);
        // print_pte_adrs(virt_addr);
        //  print_pte(pte);

        pte_cache[page] = pte;
    }

    return pte_cache[page];
}

uint64_t *get_pte_by_address(void *virt_addr)
{
    size_t page_num = virt_to_pagenum(virt_addr);
    return get_pte_by_page(page_num);
}

int pte_revoke_pages(size_t page, size_t num_pages)
{
    for (size_t i = 0; i < num_pages; i++)
    {
        // int j = 0;
        // edbgrd(pagenum_to_virt(page + i), &j, 4);
        // int res = mlock(pagenum_to_virt(page + i), 4096);
        // print_pte_adrs(pagenum_to_virt(page + i));
        // if (res != 0)
        // {
        //     info("Cannot mlock");
        // }

        uint64_t *pte = get_pte_by_page(page + i);
        if (!pte)
        {
            info("Error retrieving PTE");
            return -1; // Error retrieving PTE
        }

        int result = pte_removeperms(pte);
        if (result != 1)
        {
            info("Removing access to page %zu failed", page + i);
            // print_pte_adrs(pagenum_to_virt(page + i));

            // print_pte(get_pte_by_page(page + i));

            // uint8_t buffer[4096];
            // info("Reading contents of failed page with edbgrd");

            // edbgrd(pagenum_to_virt(page + i), buffer, 4096);

            // // Print buffer in hex format
            // printf("Page contents:\n");
            // for (int j = 0; j < 4096; j++)
            // {
            //     if (j % 16 == 0)
            //         printf("\n");
            //     printf("%02X ", buffer[j]);
            // }
            // printf("\n");

            // print_pte(get_pte_by_page(page + i));

            // // print_pte_adrs(pagenum_to_virt(page + i));
            // info("Virtual page addrs %p", pagenum_to_virt(page + i));
        }
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
            info("Error retrieving PTE");
            return -1; // Error retrieving PTE
        }

        int result = pte_restoreperms(pte);

        if (result != 1)
        {
            info("Restoring access to page %zu failed", page + i);
        }
    }
    return 0; // Success
}