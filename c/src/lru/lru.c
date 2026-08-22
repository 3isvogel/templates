#include "lru.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct PAGE {
    // Double-link
    struct PAGE* prev;
    struct PAGE* next;
    // Index of the PAGE (page key, unique)
    uint id;
    // Content of the page
    PageData data;
    // Flags of entry
    // bit0 -> allocated
    // bit1 -> grave
    int flags;
} PAGE;
typedef PAGE* Page;

#define todo(x) do { fprintf(stderr, "%s\n", #x); abort(); } while(0)

#define _FLAG_ALLOC 0
#define _FLAG_VALID 1

#define FLAG_ALLOC (1<<(_FLAG_ALLOC))
#define FLAG_VALID (1<<(_FLAG_VALID))

struct LRU {
    // Number of pages
    uint available;
    // Size of a 
    uint page_size;
    // PAGE data
    // Functions to load and store pages
    PageAcces f_load_page;
    PageAcces f_store_page;
    // Vector of pages
    PAGE* pages;
    // Needed for hash: number of buckets and size of the probe
    uint page_number;
    uint probe_size;
    // Double-link
    PAGE list;
};

// Hash stuff {{{

static inline uint32_t _roundUp32(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static inline uint64_t _roundUp64(uint64_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

 uint32_t _hash32(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

uint64_t _hash64(uint64_t x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

static inline unsigned int _isPrime(unsigned int n)
{
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) if (n % i == 0) return 0;
    return 1;
}

unsigned int _biggestPrime(unsigned int n)
{
    for (unsigned int i = n; i > 1; i--) if (_isPrime(i)) return i;
    return 0;
}

// }}}

int hash_size_init(uint size, uint* map_size, uint* probe_size)
{
    *map_size = _roundUp64(size);
    *probe_size = _biggestPrime(*map_size);
    return *probe_size;
}

Lru lru_init(uint entries, uint page_size, PageAcces f_load_page, PageAcces f_store_page)
{
    uint page_number, probe_size;
    if(!hash_size_init(entries, &page_number, &probe_size)) return NULL;
    // Allocate main structure
    Lru lru = (Lru)malloc(sizeof(LRU));
    if (!lru) return NULL;
    // Populate structure
    lru->available = entries;
    lru->page_size = page_size;
    lru->f_load_page = f_load_page;
    lru->f_store_page = f_store_page;
    lru->page_number = page_number;
    lru->probe_size = probe_size;
    lru->list.prev = &lru->list;
    lru->list.next = &lru->list;
    // Allocate pages vector
    lru->pages = (PAGE*)malloc(sizeof(PAGE) * lru->page_number);
    if (!lru->pages) { free(lru); return NULL; }
    // Clear all flags
    for(int i = 0; i < lru->page_number; i++) lru->pages[i].flags = 0;

    return lru;
}

int lru_get_page(Lru lru, void* dest, uint id)
{
    int need_load = 1;
    // TODO: add a second candidate (the page to evict if exixts) and use that instead
    // if it's found earlier in the scan chain
    struct {
        int valid,
            page;
    } candidate = {0};
    for (int i = 0, first = 0, h = _hash64(id) % lru->page_number
    ; i < lru->page_number
    ; i++, h = (h + lru->probe_size) % lru->page_number)
    {
        // PAGE not allocated: allocate now
        if ((lru->pages[h].flags & FLAG_ALLOC) == 0)
        {
            // If allocation fails, page could not be loaded
            Page page = &lru->pages[h];
            void* d = malloc(lru->page_size);
            if (!d) return 0;
            // Point to memory buffer
            page->data = d;
            // Flag as allocated and invalid (nothing is loaded)
            page->flags |= FLAG_ALLOC;
            page->flags &= ~FLAG_VALID;
            candidate.valid = 1;
            candidate.page = h;
            break;
        }
        // PAGE allocated and contains the correct index
        // Done after FLAG_ALLOC to avoid problems with initialization values
        if (lru->pages[h].id == id && lru->pages[h].flags & FLAG_VALID)
        {
            // page was found, no need to load from disk
            need_load = 0;
            candidate.valid = 1;
            candidate.page = h;
            break;
        }
        // PAGE allocated, save first grave entry found in probing for later
        // substitution
        if (first && (lru->pages[h].flags & FLAG_VALID) == 0)
        {
            candidate.valid = 1;
            candidate.page = h;
        }
    }

    Page t_page;
    if (!need_load)
    {
        // Can assume at least one node exists in the double-linked list
        assert(lru->list.prev != &lru->list);
        assert(lru->list.next != &lru->list);

        // Pop page from list
        t_page = &lru->pages[candidate.page];
        t_page->prev->next = t_page->next;
        t_page->next->prev = t_page->prev;
    }
    else
    {
        if (lru->available > 0) lru->available --;
        else
        {
            // No more entries available, needs to evict lru
            t_page = lru->list.next;
            // Assume at least one node in the list, cannot evict fictitious node
            assert(t_page != &lru->list);
            // Store page, if fails I cannot load new page
            if(lru->f_store_page(t_page->data, t_page->id) < 0) return 0;
            // Remove first page 
            t_page->prev->next = t_page->next;
            t_page->next->prev = t_page->prev;
            // Evicted page is not considered valid anymore
            t_page->flags &= ~FLAG_VALID;
        }

        // If scanning found a valid candidate, use that instead of evicted
        if (candidate.valid) t_page = &lru->pages[candidate.page];
        // Assume page is not valid, otherwise should not be here
        assert((t_page->flags & FLAG_VALID) == 0);
        // Assume page already allocated, otherwise write out of boundaries
        assert(t_page->flags & FLAG_ALLOC);
        // Set new page id
        t_page->id = id;
        // Loading from disk fails, cannot continue
        if(lru->f_load_page(t_page->data, t_page->id) < 0) return 0;
        t_page->flags |= FLAG_VALID;
    }
    t_page->next = &(lru->list);
    t_page->prev = lru->list.prev;
    t_page->next->prev = t_page;
    t_page->prev->next = t_page;
    // Actually copy the memory
    memcpy(dest, t_page->data, lru->page_size);
    return 1;
}

int lru_destroy(Lru lru)
{
    for(int i = 0; i < lru->page_number; i++) {
        Page page = &lru->pages[i];
        if (page->flags & FLAG_ALLOC) {
            if (page->flags & FLAG_VALID)
                // Store valid pages
                if(lru->f_store_page(page->data, page->id) < 0) return 0;
            // Free allocated pages
            free(lru->pages[i].data);
        }
    }
    // Free pages and lru
    free(lru->pages);
    free(lru);
    return 1;
}
