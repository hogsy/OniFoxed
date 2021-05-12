/*
LRAR_CACHE.H
*/

#ifndef LRAR_CACHE_H
#define LRAR_CACHE_H

typedef void (*lrar_new_block_proc)(void *user_data, short block_index);
typedef void (*lrar_purge_block_proc)(void *user_data);

typedef struct lrar_cache lrar_cache;
typedef struct lrar_cache_block lrar_cache_block;

lrar_cache *lrar_new(char *name, unsigned long minimum_address, unsigned long maximum_address,
	short block_count, short alignment_bit, short boundary_bit,
	lrar_new_block_proc new_block, lrar_purge_block_proc purge_block);
void lrar_dispose(lrar_cache *cache);
void lrar_flush(lrar_cache *cache);

short lrar_allocate(lrar_cache *cache, long size, char *inDebugInfo, void *user_data);
void lrar_deallocate(lrar_cache *cache, short block_index);

unsigned long lrar_block_address(lrar_cache *cache, short block_index);

UUtBool lrar_full(lrar_cache *cache);

void verify_lrar_cache(
	lrar_cache *cache);

#endif /* LRAR_CACHE_H */
