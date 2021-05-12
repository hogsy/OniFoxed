/*
LRAR_CACHE2.C

Saturday October 4, 1997 9:09 PM
	myth waning.

Thursday August 31, 2000 11:28 AM
	oni waning.

*/

#include "bfw_cseries.h"
#include "lrar_cache.h"

#include <string.h>

/* ---------- constants */

enum
{
	LRAR_CACHE_SIGNATURE= 'lrar',
	LRAR_CACHE_BLOCK_SIGNATURE= 'Rblk',
	LRAR_CACHE_DEALLOCATED_BLOCK_SIGNATURE= 'Dblk',
	
	MAXIMUM_LRAR_CACHE_NAME_LENGTH= 31
};

/* ---------- structures */

typedef UUtBool boolean;

struct lrar_cache_block
{
	void *user_data;
	unsigned long signature;

	unsigned long address;
	long size;


#if defined(DEBUGGING) && DEBUGGING
	char debug_info[128];
#endif
};

struct lrar_cache
{
	char name[MAXIMUM_LRAR_CACHE_NAME_LENGTH+1];

	short alignment_bit; // align to 2^alignment_bit byte boundaries
	short boundary_bit; // don't allow allocations to span 2^boundary_bit boundaries

	unsigned long minimum_address, maximum_address;
	long size; // cache->maximum_address-cache->minimum_address

	struct lrar_cache_block *blocks;
	short oldest_block_index; // the next block index to deallocate when we need space
	short last_allocated_block_index; // the last block index we allocated
	short block_count;

	lrar_new_block_proc new_block;
	lrar_purge_block_proc purge_block;

	unsigned long signature;
};

/* ---------- private prototypes */

#define CACHE_BLOCK_OFFSET(cache, block) ((byte *)(block) - (byte *)(cache)->base_address)

#ifdef DEBUG
void verify_lrar_cache(struct lrar_cache *cache);
static void verify_lrar_cache_block(struct lrar_cache *cache, struct lrar_cache_block *block);
static struct lrar_cache_block *get_lrar_cache_block(struct lrar_cache *cache, short block_index);
#else
#define verify_lrar_cache(c)
#define verify_lrar_cache_block(c, b)
#define get_lrar_cache_block(c, i) ((c)->blocks + (i))
#endif

static void lrar_default_new_block_proc(void *user_data, short block_index);
static void lrar_default_purge_block_proc(void *user_data);

static void lrar_purge_block(struct lrar_cache *cache, struct lrar_cache_block *block);

/* ---------- code */

struct lrar_cache *lrar_new(
	char *name,
	unsigned long minimum_address,
	unsigned long maximum_address,
	short block_count,
	short alignment_bit,
	short boundary_bit,
	lrar_new_block_proc new_block,
	lrar_purge_block_proc purge_block)
{
	struct lrar_cache *cache= (struct lrar_cache *) UUrMemory_Block_New(sizeof(struct lrar_cache));

	assert(minimum_address<maximum_address);

	// align minimum address	
	if (minimum_address&((1<<alignment_bit)-1))
	{
		minimum_address= (minimum_address|((1<<alignment_bit)-1))+1;
	}

	if (!new_block || !purge_block)
	{
		new_block= lrar_default_new_block_proc;
		purge_block= lrar_default_purge_block_proc;
	}

	assert(alignment_bit>=0);
	assert(boundary_bit==NONE || boundary_bit>=0);
	assert(block_count>0);

	if (cache)
	{
		struct lrar_cache_block *blocks= (struct lrar_cache_block *) UUrMemory_Block_New(block_count*sizeof(struct lrar_cache_block));

		if (blocks)
		{
			UUrMemory_Clear(cache, sizeof(struct lrar_cache));
			UUrMemory_Clear(blocks, block_count*sizeof(struct lrar_cache_block));
			
			strncpy(cache->name, name, MAXIMUM_LRAR_CACHE_NAME_LENGTH);
			cache->name[MAXIMUM_LRAR_CACHE_NAME_LENGTH]= 0;

			cache->minimum_address= minimum_address;
			cache->maximum_address= maximum_address;
			cache->size= cache->maximum_address-cache->minimum_address;
			cache->alignment_bit= alignment_bit;
			cache->boundary_bit= boundary_bit;

			cache->blocks= blocks;
			cache->oldest_block_index= NONE;
			cache->last_allocated_block_index= NONE;
			cache->block_count= block_count;

			cache->purge_block= purge_block;
			cache->new_block= new_block;

			cache->signature= LRAR_CACHE_SIGNATURE;

			verify_lrar_cache(cache);
		}
		else
		{
			UUrMemory_Block_Delete(cache);
			
			cache= (struct lrar_cache *) NULL;
		}
	}
	
	return cache;
}

void lrar_dispose(
	struct lrar_cache *cache)
{
	verify_lrar_cache(cache);
	
	UUrMemory_Block_Delete(cache->blocks);
	UUrMemory_Block_Delete(cache);
	
	return;
}

void lrar_flush(
	struct lrar_cache *cache)
{
	short block_index;
	
	verify_lrar_cache(cache);

	block_index= cache->oldest_block_index;
	while (block_index!=NONE)
	{
		struct lrar_cache_block *block= get_lrar_cache_block(cache, block_index);

		if (block->signature == LRAR_CACHE_BLOCK_SIGNATURE) {
			lrar_purge_block(cache, block);
		}

		if (block_index==cache->last_allocated_block_index)
		{
			block_index= NONE;
		}
		else
		{
			block_index+= 1;
			if (block_index==cache->block_count) block_index= 0;
		}
	}

	cache->oldest_block_index= NONE;
	cache->last_allocated_block_index= NONE;
	
	return;
}

short lrar_allocate(
	struct lrar_cache *cache,
	long size,
	char *debug_info,
	void *user_data)
{
	short new_block_index= NONE;

	verify_lrar_cache(cache);

	// add the size of a block header and 4-byte align it
	if (size&((1<<cache->alignment_bit)-1)) size= (size|((1<<cache->alignment_bit)-1))+1;
	
	if (size>=0 && size<=cache->size)
	{
		short oldest_block_index= cache->oldest_block_index;
		
		boolean done= UUcFalse;

		new_block_index= cache->last_allocated_block_index==NONE ? 0 : (cache->last_allocated_block_index+1);
		if (new_block_index>=cache->block_count) new_block_index= 0;
		
		while (!done)
		{
			unsigned long new_block_address;
			unsigned long adjusted_new_block_address; // to compensate for boundary crossing

			// compute the base address of the new block			
			if (cache->last_allocated_block_index==NONE)
			{
				new_block_address= cache->minimum_address;
			}
			else
			{
				struct lrar_cache_block *newest_block= get_lrar_cache_block(cache, cache->last_allocated_block_index);

				new_block_address= newest_block->address + newest_block->size;
			}

			// adjust it, if necessary, to avoid crossing a boundary
			adjusted_new_block_address= new_block_address;
			if (cache->boundary_bit!=NONE)
			{
				unsigned long boundary_mask= ~((1<<cache->boundary_bit)-1);

				if ((new_block_address&boundary_mask) != ((new_block_address+size)&boundary_mask))
				{
					adjusted_new_block_address= (new_block_address&boundary_mask) + (1<<cache->boundary_bit);
				}
			}

			if (oldest_block_index!=NONE)
			{
				struct lrar_cache_block *oldest_block= get_lrar_cache_block(cache, oldest_block_index);

				// the address space of this new block would overlap the address space of
				// the oldest allocated block or we don't have any free blocks, so purge
				// the oldest allocated block; keep doing this until we run out of blocks
				// between us and the end of the cache or we find enough space
				while (oldest_block_index==new_block_index ||
					(new_block_address<=oldest_block->address && adjusted_new_block_address + size > oldest_block->address))
				{
					if (oldest_block->signature == LRAR_CACHE_BLOCK_SIGNATURE) {
						lrar_purge_block(cache, oldest_block);
					}
					
					oldest_block_index+= 1;
					if (oldest_block_index>=cache->block_count) oldest_block_index= 0;

					oldest_block= get_lrar_cache_block(cache, oldest_block_index);
				}	
			}
			
			if (adjusted_new_block_address + size > cache->maximum_address)
			{
				// this block would fall off the edge of the cache, wrap
				
				cache->last_allocated_block_index= NONE;
			}
			else
			{
				struct lrar_cache_block *new_block= cache->blocks + new_block_index;

#ifdef DEBUG
				// make certain that this block won't overlap any existing blocks
				assert(adjusted_new_block_address>=cache->minimum_address &&
					adjusted_new_block_address+size<=cache->maximum_address);
				{
					short test_block_index;

					for (test_block_index= 0; test_block_index<cache->block_count; ++test_block_index)
					{
						struct lrar_cache_block *test_block= cache->blocks + test_block_index;

						if (test_block->signature==LRAR_CACHE_BLOCK_SIGNATURE)
						{
							assert(adjusted_new_block_address>=test_block->address+test_block->size ||
								adjusted_new_block_address+size<=test_block->address);
						}
					}
				}
#endif

				new_block->signature= LRAR_CACHE_BLOCK_SIGNATURE;
				new_block->size= size;
				new_block->address= adjusted_new_block_address;
				new_block->user_data= user_data;
#if defined(DEBUGGING) && DEBUGGING
				if (debug_info)
				{
					int length= strlen(debug_info)+1;

					assert(length <= 128);
					strncpy(new_block->debug_info, debug_info, length);
				}
#endif
				cache->new_block(user_data, new_block_index);

				done= UUcTrue;
			}
		}

		cache->last_allocated_block_index= new_block_index;

		if (oldest_block_index==NONE) oldest_block_index= new_block_index;
		cache->oldest_block_index= oldest_block_index;
	}

	return new_block_index;
}

unsigned long lrar_block_address(
	struct lrar_cache *cache,
	short block_index)
{
	return get_lrar_cache_block(cache, block_index)->address;
}

void lrar_deallocate(
	struct lrar_cache *cache,
	short block_index)
{
	verify_lrar_cache(cache);
	lrar_purge_block(cache, get_lrar_cache_block(cache, block_index));

	return;
}

/* ---------- private code */

static void lrar_default_new_block_proc(
	void *user_data,
	short block_index)
{
	*(short *)user_data= block_index;
	
	return;
}

static void lrar_default_purge_block_proc(
	void *user_data)
{
	*(short *)user_data= NONE;
	
	return;
}

static void lrar_purge_block(
	struct lrar_cache *cache,
	struct lrar_cache_block *block)
{
	// CB: moved the signature check and clear inside this function to fix spurious
	// assertions that were appearing because lrar_flush and lrar_deallocate were
	// calling this function and not clearing the signature  - september 27, 2000
	assert(block->signature == LRAR_CACHE_BLOCK_SIGNATURE);
	if (block->user_data)
	{
		cache->purge_block(block->user_data);
		block->user_data= NULL;
	}
	block->signature= LRAR_CACHE_DEALLOCATED_BLOCK_SIGNATURE;

	return;
}

#ifdef DEBUG
static struct lrar_cache_block *get_lrar_cache_block(
	struct lrar_cache *cache,
	short block_index)
{
	struct lrar_cache_block *block;

	verify_lrar_cache(cache);

	assert(block_index>=0 && block_index<cache->block_count);

	block= cache->blocks + block_index;
	verify_lrar_cache_block(cache, block);
	
	return block;
}

static void verify_lrar_cache_block(
	struct lrar_cache *cache,
	struct lrar_cache_block *block)
{
	boolean valid= UUcFalse;

	if ((block->signature==LRAR_CACHE_BLOCK_SIGNATURE) || (block->signature==LRAR_CACHE_DEALLOCATED_BLOCK_SIGNATURE))
	{
		if (block->size>=0 && block->size<cache->size)
		{
			if (block->address>=cache->minimum_address && block->address+block->size<=cache->maximum_address)
			{
				valid= UUcTrue;
			}
		}
	}
	
	vassert(valid, csprintf(temporary, "lrar cache %s @%p block @%p appears to be corrupt",
		cache->name, cache, block));
	
	return;
}

void verify_lrar_cache(
	struct lrar_cache *cache)
{
	boolean valid= UUcFalse;
	
	if (cache->signature==LRAR_CACHE_SIGNATURE &&
		cache->minimum_address<cache->maximum_address &&
		cache->size>0 &&
		cache->block_count>0)
	{
		valid= UUcTrue;
	}

	vassert(valid, csprintf(temporary, "lrar cache %s @%p appears to be corrupt",
		cache->name, cache));
	
	return;
}
#endif
