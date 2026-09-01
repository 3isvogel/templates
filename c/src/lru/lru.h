#ifndef SRC_LRU_LRU_H
#define SRC_LRU_LRU_H

// Move to <utils/utils.h>
#include <utils/types.h>

typedef char* PageData;
typedef int (*PageAcces)(PageData data, uint index);

typedef struct LRU LRU;
typedef LRU* Lru;

/**
 * @brief Initialize lru cache data structure
 *
 * @param page_number number of pages in the cache
 * @param page_size cache page size
 * @param f_load_page function to load pages (must return < 0 on fail)
 * @param f_store_page function to store pages (must return < 0 on fail)
 * @return pointer to LRU structure, NULL on fail
 */
Lru  lru_init(uint page_number, uint page_size, PageAcces f_load_page, PageAcces f_store_page);

/**
 * @brief Stores all active pages and destroys LRU
 *
 * @param lru lru reference
 * @return positive on success, 0 otherwise (f_store_page fails)
 */
int lru_destroy(Lru lru);

/**
 * @brief Loads content of page `index` into dest
 *
 * @param lru lru reference
 * @param id unique page id (passed to f_load_page and f_store_page)
 * @return address of page on success, NULL otherwise
 */
void* lru_get_page(Lru lru, uint id);

#endif // SRC_LRU_LRU_H
