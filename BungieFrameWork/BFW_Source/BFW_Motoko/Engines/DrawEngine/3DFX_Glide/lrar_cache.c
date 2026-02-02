/*
LRAR_CACHE.C

Saturday October 4, 1997 9:09 PM
	myth waning.
*/

#if 0 // lrar_cache2.c

#include "bfw_cseries.h"
#include "lrar_cache.h"

#include <string.h>

/* ---------- constants */

enum
{
	LRAR_CACHE_SIGNATURE= 'lrar',
	LRAR_CACHE_BLOCK_SIGNATURE= 'Rblk',

	MAXIMUM_LRAR_CACHE_NAME_LENGTH= 31
};

/* ---------- structures */

struct lrar_cache_block
{
	char debug_info[128];
	void *user_data;
	unsigned long signature;

	unsigned long address;
	long size;
};

struct lrar_cache
{
	char name[MAXIMUM_LRAR_CACHE_NAME_LENGTH+1];

	short alignment_bit; // align to 2^alignment_bit byte boundaries
	short boundary_bit; // don't allow allocations to span 2^boundary_bit boundaries

	unsigned long minimum_address, maximum_address;
	long size; // cache->maximum_address-cache->minimum_address

	lrar_cache_block *blocks;
	short oldest_block_index; // the next block index to deallocate when we need space
	short last_allocated_block_index; // the last block index we allocated
	short max_block_count;				// maximum number of allocated blocks
	short cur_block_count;

	lrar_new_block_proc new_block;
	lrar_purge_block_proc purge_block;

	unsigned long signature;
};

/* ---------- private prototypes */

#define CACHE_BLOCK_OFFSET(cache, block) ((byte *)(block) - (byte *)(cache)->base_address)

#ifdef DEBUG
void verify_lrar_cache(lrar_cache *cache);
static void verify_lrar_cache_block(lrar_cache *cache, lrar_cache_block *block);
static lrar_cache_block *get_lrar_cache_block(lrar_cache *cache, short block_index);
#else
#define verify_lrar_cache(c)
#define verify_lrar_cache_block(c, b)
#define get_lrar_cache_block(c, i) ((c)->blocks + (i))
#endif

static void lrar_default_new_block_proc(void *user_data, short block_index);
static void lrar_default_purge_block_proc(void *user_data);

/* ---------- code */

 lrar_cache *lrar_new(
	char *name,
	unsigned long minimum_address,
	unsigned long maximum_address,
	short max_block_count,
	short alignment_bit,
	short boundary_bit,
	lrar_new_block_proc new_block,
	lrar_purge_block_proc purge_block)
{
	 lrar_cache *cache= ( lrar_cache *) malloc(sizeof( lrar_cache));

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
	assert(max_block_count>0);

	if (cache)
	{
		 lrar_cache_block *blocks= ( lrar_cache_block *) malloc(max_block_count*sizeof( lrar_cache_block));

		if (blocks)
		{
			UUrMemory_Clear(cache, sizeof( lrar_cache));
			UUrMemory_Clear(blocks, max_block_count*sizeof( lrar_cache_block));

			UUrString_Copy(cache->name, name, MAXIMUM_LRAR_CACHE_NAME_LENGTH);
			cache->name[MAXIMUM_LRAR_CACHE_NAME_LENGTH]= 0;

			cache->minimum_address= minimum_address;
			cache->maximum_address= maximum_address;
			cache->size= cache->maximum_address-cache->minimum_address;
			cache->alignment_bit= alignment_bit;
			cache->boundary_bit= boundary_bit;

			cache->blocks= blocks;
			cache->oldest_block_index= NONE;
			cache->last_allocated_block_index= NONE;
			cache->max_block_count= max_block_count;
			cache->cur_block_count = 0;

			cache->purge_block= purge_block;
			cache->new_block= new_block;

			cache->signature= LRAR_CACHE_SIGNATURE;

			verify_lrar_cache(cache);
		}
		else
		{
			free(cache);

			cache= ( lrar_cache *) NULL;
		}
	}

	return cache;
}

void lrar_dispose(
	 lrar_cache *cache)
{
	verify_lrar_cache(cache);

	free(cache->blocks);
	free(cache);

	return;
}

static void lrar_internal_purge_oldest_block(
	 lrar_cache *cache)
{
	 lrar_cache_block *oldest_block;

	assert(cache->cur_block_count > 0);
	assert(cache->oldest_block_index != NONE);

	oldest_block = get_lrar_cache_block(cache, cache->oldest_block_index);

	cache->purge_block(oldest_block->user_data);
	UUrMemory_Clear(oldest_block, sizeof( lrar_cache_block));

	cache->cur_block_count -= 1;

	if (0 == cache->cur_block_count)
	{
		// no more blocks
		cache->last_allocated_block_index = NONE;
		cache->oldest_block_index = NONE;
	}
	else
	{
		// bump oldest_block_index mod cache->max_block_count
		cache->oldest_block_index += 1;
		assert(cache->oldest_block_index <= cache->max_block_count);

		if (cache->oldest_block_index >= cache->max_block_count) cache->oldest_block_index= 0;
	}
}


void lrar_flush(
	 lrar_cache *cache)
{
	verify_lrar_cache(cache);

	while(cache->cur_block_count > 0)
	{
		lrar_internal_purge_oldest_block(cache);
	}

	verify_lrar_cache(cache);

	return;
}

void lrar_deallocate( lrar_cache *cache, short block_index)
{
	 lrar_cache_block *block= get_lrar_cache_block(cache, block_index);

	verify_lrar_cache(cache);

	cache->purge_block(block->user_data);
	block->user_data = NULL;

	verify_lrar_cache(cache);
}

short lrar_allocate(
	 lrar_cache *cache,
	long size,
	char *inDebugInfo,
	void *user_data)
{
	short new_block_index= NONE;

	UUmAssert(size > 0);

	verify_lrar_cache(cache);

	// add the size of a block header and 4-byte align it
	if (size&((1<<cache->alignment_bit)-1)) size= (size|((1<<cache->alignment_bit)-1))+1;

	if (size>=0 && size<=cache->size)
	{
		UUtBool wrapped = UUcFalse;
		UUtBool done= UUcFalse;

		// we have run out of blocks so purge the entire cache
		if (cache->cur_block_count == cache->max_block_count)
		{
			lrar_flush(cache);
		}

		if (NONE == cache->last_allocated_block_index)
		{
			new_block_index = 0;
		}
		else
		{
			new_block_index = cache->last_allocated_block_index + 1;
			if (new_block_index>=cache->max_block_count) new_block_index= 0;
		}

		while (!done)
		{
			unsigned long new_block_address;

			// compute the base address of the new block
			if (cache->last_allocated_block_index==NONE)
			{
				new_block_address= cache->minimum_address;
			}
			else
			{
				 lrar_cache_block *newest_block= get_lrar_cache_block(cache, cache->last_allocated_block_index);

				new_block_address= newest_block->address + newest_block->size;
			}

			// adjust it, if necessary, to avoid crossing a boundary
			if (cache->boundary_bit!=NONE)
			{
				unsigned long boundary_mask= ~((1<<cache->boundary_bit)-1);

				if ((new_block_address&boundary_mask) != ((new_block_address+size)&boundary_mask))
				{
					new_block_address= (new_block_address&boundary_mask) + (1<<cache->boundary_bit);
				}
			}

			//  handle wrapping & the case of an empty cache
			if (cache->oldest_block_index != NONE)
			{
				if ((new_block_address + size) > cache->maximum_address)
				{
					lrar_cache_block *oldest_block = get_lrar_cache_block(cache, cache->oldest_block_index);

					// if we wrapped this time, purge all blocks on the end of the cache
					while ((cache->oldest_block_index != NONE) && (oldest_block->address > new_block_address))
					{
						oldest_block = get_lrar_cache_block(cache, cache->oldest_block_index);
						lrar_internal_purge_oldest_block(cache);
					}

					new_block_address = cache->minimum_address;
				}
			}
			else
			{
				new_block_address = cache->minimum_address;
			}

			// if there are any blocks to purge we may need to purge
			if (cache->oldest_block_index != NONE)
			{
				lrar_cache_block *oldest_block = get_lrar_cache_block(cache, cache->oldest_block_index);

				while ((new_block_address<=oldest_block->address) && (new_block_address + size >= oldest_block->address))
				{
					// loop until we wrap or have a free contiguous block
					lrar_internal_purge_oldest_block(cache);
					oldest_block = get_lrar_cache_block(cache, cache->oldest_block_index);

					assert(oldest_block != NULL);
				}
			}

			{
				 lrar_cache_block *new_block= cache->blocks + new_block_index;

				// this block will fit before next_whole_block

				assert(new_block_address>=cache->minimum_address &&
					new_block_address+size<=cache->maximum_address);
				{
					short test_block_index;

					for (test_block_index= 0; test_block_index<cache->max_block_count; ++test_block_index)
					{
						 lrar_cache_block *test_block= cache->blocks + test_block_index;

						if (test_block->signature==LRAR_CACHE_BLOCK_SIGNATURE)
						{
							assert(new_block_address>=test_block->address+test_block->size ||
								new_block_address+size<=test_block->address);
						}
					}
				}


				strncpy(new_block->debug_info, inDebugInfo, 128);
				new_block->signature= LRAR_CACHE_BLOCK_SIGNATURE;
				new_block->size= size;
				new_block->address= new_block_address;
				new_block->user_data= user_data;
				cache->new_block(user_data, new_block_index);
				cache->cur_block_count += 1;

				assert(cache->cur_block_count <= cache->max_block_count);

				done= UUcTrue;
			}
		}

		cache->last_allocated_block_index= new_block_index;
		if (cache->oldest_block_index==NONE) cache->oldest_block_index= new_block_index;
	}

	return new_block_index;
}

unsigned long lrar_block_address(
	 lrar_cache *cache,
	short block_index)
{
	verify_lrar_cache(cache);

	return get_lrar_cache_block(cache, block_index)->address;
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
	if (NULL != user_data) {
		*(short *)user_data= NONE;
	}

	return;
}

#ifdef DEBUG
static  lrar_cache_block *get_lrar_cache_block(
	 lrar_cache *cache,
	short block_index)
{
	 lrar_cache_block *block;

	verify_lrar_cache(cache);

	assert(block_index>=0 && block_index<cache->max_block_count);

	block= cache->blocks + block_index;
	verify_lrar_cache_block(cache, block);

	return block;
}

static void verify_lrar_cache_block(
	 lrar_cache *cache,
	 lrar_cache_block *block)
{
	UUtBool valid= UUcFalse;

	if (block->signature==LRAR_CACHE_BLOCK_SIGNATURE &&
		block->size>=0 && block->size<cache->size)
	{
		if (block->address>=cache->minimum_address && block->address+block->size<=cache->maximum_address)
		{
			valid= UUcTrue;
		}
	}

	vassert(valid, csprintf(temporary, "lrar cache %s @%p block @%p appears to be corrupt",
		cache->name, cache, block));

	return;
}

void verify_lrar_cache(
	 lrar_cache *cache)
{
	short new_block_index= NONE;
	short itr;
	UUtBool valid= UUcFalse;

	if (cache->signature==LRAR_CACHE_SIGNATURE &&
		cache->minimum_address<cache->maximum_address &&
		cache->size>0 &&
		cache->max_block_count>0)
	{
		valid= UUcTrue;
	}


	for(itr = 0; itr < cache->cur_block_count; itr++)
	{
		short block_index = (cache->oldest_block_index + itr) % cache->max_block_count;
		 lrar_cache_block *block = cache->blocks + block_index;

		verify_lrar_cache_block(cache, block);

		// CB: only check user data if we know what it's being used for
		if (block->user_data && (cache->new_block == lrar_default_new_block_proc))
		{
			short user_data;

			user_data = *((short *) (block->user_data));
			UUmAssert(block_index == user_data);
		}
	}

	vassert(valid, csprintf(temporary, "lrar cache %s @%p appears to be corrupt",
		cache->name, cache));

	return;
}
#endif

#endif // lrar_cache2.c
