#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cachesim.h"

/* Write Back with Write Allocate:  */
 /* on hits it writes to cache setting �dirty� bit for the block, main memory is not updated;  */
 /* on misses it updates the block in main memory and brings the block to the cache;  */
 /* Subsequent writes to the same block, if the block originally caused a miss, will hit in the cache next time, setting dirty bit for the block. That will eliminate extra memory accesses and result in very efficient execution compared with Write Through with Write Allocate combination. */

struct cache L1, VC, MM;

// common attribute to both caches
uint64_t bytes_per_block;
uint64_t _b;

uint64_t _s;

uint64_t cur_time = 0;

// Given a set defined by first_block_in_set and blocks_per_set set, return a
// pointer to the block that should be evicted. See update_time_on_access for
// more details. This function can return invalid blocks.
struct block * evict_choice(struct block * first_block_in_set, uint64_t blocks_per_set) {
	struct block * choice = first_block_in_set, * this = first_block_in_set;
	uint64_t i;
	for (i = 0; i < blocks_per_set; i++, this++) {
		if (!this->valid)
			return this;
		if (this->time < choice->time)
			choice = this;
	}
	return choice;
}

void print_cache_info(struct cache * c) {
	printf("the %s cache\n", cache_name[c->level]);
	printf("============\n");
	printf("\tbytes: %" PRIu64 "\n", bytes_per_block * c->n_blocks);
	printf("\tbytes/block: %" PRIu64 "\n", bytes_per_block);
	printf("\tblocks: %" PRIu64 "\n",c->n_blocks );
	printf("\tblocks/set: %" PRIu64 "\n", c->blocks_per_set);
	printf("\tsets: %" PRIu64 "\n", N_SETS_FROM_CACHE(c));
	printf("\tvictim cache: %s\n", (c->victim_cache == NULL) ? "null" : cache_name[c->victim_cache->level]);
	printf("\teviction policy: %s\n", (c->update_time_on_access) ? "lru" : "fifo");
	printf("\tblocks to prefetch: %" PRIu64 "\n\n", c->n_prefetch_blocks);
}

void print_block_info(struct block * b) {
	printf("[block tag:0x%" PRIx64 " dirty:%s valid:%s time:%d]\n",
	       b->tag,
	       b->dirty ? "1" : "0",
	       b->valid ? "1" : "0",
	       b->time);
}

/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c The total number of bytes for data storage is 2^C
 * @b The size of a single cache line in bytes is 2^B
 * @s The number of blocks in each set is 2^S
 * @v The number of blocks in the victim cache is V
 * @k The prefetch distance is K
 */
void setup_cache(uint64_t c, uint64_t b, uint64_t s, uint64_t v, uint64_t k) {
	// this is a common attribute to both caches
	bytes_per_block = BYTES_PER_BLOCK(b);
	_b = b;
	_s = s;

	L1.level = L1_level;
	L1.n_blocks = N_BLOCKS(c,b);
	L1.blocks_per_set = BLOCKS_PER_SET(s);
	L1.n_prefetch_blocks = k;
	L1.update_time_on_access = 1;
	L1.victim_cache = v ? &VC : NULL;
	L1.blocks = calloc(L1.n_blocks, sizeof(struct block));
	print_cache_info(&L1);

	if (v) {
		VC.level = VC_level;
		VC.n_blocks = v;
		VC.blocks_per_set = v;
		VC.n_prefetch_blocks = 0;
		VC.update_time_on_access = 0;
		VC.victim_cache = NULL;
		VC.blocks = calloc(VC.n_blocks, sizeof(struct block));
		print_cache_info(&VC);
	}
}

void copy_block(struct block * from, struct block * to){
	memcpy(to, from, sizeof(struct block));
}

/**
 * Subroutine that simulates the cache one trace event at a time.

 * XXX: You're responsible for completing this routine
 *
 * @rw The type of event. Either READ or WRITE
 * @address  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char rw, uint64_t address, struct cache_stats_t* p_stats) {
	(rw == READ) ? p_stats->reads++ : p_stats->writes++;

	general_cache_access(&L1, rw, address, p_stats);
}

void general_cache_access(struct cache * cache, char rw, uint64_t addr, struct cache_stats_t* p_stats) {
	struct block * first_block_in_set = get_addr_set(cache, addr), * evict_block;
	struct block * block_in_cache = find_block_in_set(first_block_in_set, cache->blocks_per_set, addr);

	if (block_in_cache != NULL){
		// hit
		if (rw == WRITE)
			block_in_cache->dirty = 1;
		if (cache->update_time_on_access)
			block_in_cache->time = cur_time;
	} else {
		// miss
		if (cache->level == L1_level) {
			p_stats->misses++;
			(rw == READ) ? p_stats->read_misses++ : p_stats->write_misses++;
			(rw == READ) ? p_stats->read_misses_combined++ : p_stats->write_misses_combined++;
			if (cache->victim_cache == NULL)
				p_stats->vc_misses++;

		} else {
			p_stats->vc_misses++;
			(rw == READ) ? p_stats->read_misses_combined++ : p_stats->write_misses_combined++;
		}

		if (cache->victim_cache != NULL) {
			general_cache_access(cache->victim_cache, rw, addr, p_stats);
		} else {
			evict_block = evict_choice(first_block_in_set, cache->blocks_per_set);
			if (evict_block->dirty) {
				p_stats->write_backs++;
				p_stats->bytes_transferred += bytes_per_block;
			}
			overwrite_block_for_new_addr(evict_block, addr);
			p_stats->bytes_transferred += bytes_per_block;
			if (rw == WRITE)
				//ugh, calling it evict_block still after it's
				//been overwritten is a bit confusing to the
				//programmer
				evict_block->dirty = 1;
		}
	}
	cur_time++;
}

void overwrite_block_for_new_addr(struct block * block_to_overwrite, uint64_t new_addr) {
	block_to_overwrite->tag = BLOCK_TAG_FROM_ADDR(new_addr);
	block_to_overwrite->dirty = 0;
	block_to_overwrite->valid = 1;
	block_to_overwrite->time = cur_time;
}

struct block * get_addr_set(struct cache * cache, uint64_t addr) {
	uint64_t si = SET_INDEX(addr, N_SETS_FROM_CACHE(cache));
	struct block * first_block_in_set = cache->blocks + si * cache->blocks_per_set;
	return first_block_in_set;
}

struct block * find_block_in_set(struct block * first_block_in_set, uint64_t blocks_per_set, uint64_t addr) {
	uint64_t addr_tag = BLOCK_TAG_FROM_ADDR(addr);
	struct block * choice = first_block_in_set, * this = first_block_in_set;
	uint64_t i;
	for (i = 0; i < blocks_per_set; i++, this++) {
		if (addr_tag == this->tag)
			return this;
	}
	return NULL;
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(struct cache_stats_t *p_stats) {
	p_stats->accesses = p_stats->reads + p_stats->writes;

	p_stats->miss_rate = (double) p_stats->misses / p_stats->accesses;
	p_stats->hit_time = 2.0 + 0.2 * _s;
	p_stats->miss_penalty = 200; // i don't know why we're filling this in if this never changes....
	p_stats->avg_access_time = p_stats->hit_time + p_stats->miss_rate * p_stats->miss_penalty;

	free(L1.blocks);
	free(VC.blocks);
}
