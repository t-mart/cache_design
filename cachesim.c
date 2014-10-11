#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cachesim.h"

/* Write Back with Write Allocate:  */
 /* on hits it writes to cache setting �dirty� bit for the block, main memory is not updated;  */
 /* on misses it updates the block in main memory and brings the block to the cache;  */
 /* Subsequent writes to the same block, if the block originally caused a miss, will hit in the cache next time, setting dirty bit for the block. That will eliminate extra memory accesses and result in very efficient execution compared with Write Through with Write Allocate combination. */

struct cache L1, VC;

// common attribute to both caches
uint64_t bytes_per_block;
uint64_t _b;

uint64_t _s;

uint64_t _k;
char prefetch_d = 0;
uint64_t prefetch_last_miss_block = 0;
uint64_t prefetch_pending_stride = 0;

char VC_enabled = 0;
char prefetch_enabled = 0;

uint64_t cur_time = 0;

// Given a set defined by first_block_in_set and blocks_per_set set, return a
// pointer to the block that should be evicted. See update_time_on_access for
// more details. This function will return the first invalid block it sees if
// one exists in the set.
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
	bytes_per_block = BYTES_PER_BLOCK(b);
	_b = b;
	_s = s;
	_k = k;

	L1.level = L1_level;
	L1.n_blocks = N_BLOCKS(c,b);
	L1.blocks_per_set = BLOCKS_PER_SET(s);
	L1.blocks = calloc(L1.n_blocks, sizeof(struct block));

	if (v) {
		VC.level = VC_level;
		VC.n_blocks = v;
		VC.blocks_per_set = v;
		VC.blocks = calloc(VC.n_blocks, sizeof(struct block));
	}

	VC_enabled = v;
	prefetch_enabled = k;
}

void copy_block(struct block * from, struct block * to) {
	memcpy(to, from, sizeof(struct block));
}

void swap_blocks(struct block * a, struct block * b) {
	struct block temp;
	copy_block(a, &temp);
	copy_block(b, a);
	copy_block(&temp, b);
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

	L1_cache_access(rw, address, p_stats);
	cur_time++;
}

void L1_cache_access(char rw, uint64_t addr, struct cache_stats_t* p_stats) {
	struct block * first_block_in_set = get_addr_set(&L1, addr), * evict_block;
	struct block * block_in_cache = find_block_in_set(first_block_in_set, L1.blocks_per_set, addr);

	if (block_in_cache != NULL){
		// hit
		if (rw == WRITE)
			block_in_cache->dirty = 1;
		block_in_cache->time = cur_time;
	} else {
		// miss
		p_stats->misses++;
		(rw == READ) ? p_stats->read_misses++ : p_stats->write_misses++;
		if (VC_enabled) {
			VC_cache_access(rw, addr, p_stats, first_block_in_set);
		} else {
			p_stats->vc_misses++; //for whatever reason, we incr vc_misses even when !VC_enabled
			evict_block = evict_choice(first_block_in_set, L1.blocks_per_set);
			if (evict_block->dirty) {
				// dirty block back to mem
				p_stats->write_backs++;
				p_stats->bytes_transferred += bytes_per_block;
			}
			overwrite_block_for_new_addr(evict_block, addr);
			p_stats->bytes_transferred += bytes_per_block; //new block from mem to L1
			if (rw == WRITE)
				//ugh, calling it evict_block still after it's
				//been overwritten is a bit confusing to the
				//programmer
				evict_block->dirty = 1;
		}
	}
}

void VC_cache_access(char rw, uint64_t addr, struct cache_stats_t* p_stats, struct block * first_block_in_set_of_L1) {
	struct block * block_in_cache = find_block_in_set(VC.blocks, VC.blocks_per_set, addr);
	struct block * L1_evict_block = evict_choice(first_block_in_set_of_L1, L1.blocks_per_set);
	struct block * VC_evict_block;

	if (block_in_cache != NULL){
		// hit, promotion from VC to L1
		if (rw == WRITE)
			block_in_cache->dirty = 1;
		block_in_cache->time = cur_time; //this is MRU
		L1_evict_block->time = cur_time; //this is going into the FIFO VC
		swap_blocks(L1_evict_block, block_in_cache);
	} else {
		// miss, evict L1 LRU to VC and evict VC FIFO completely from VC
		p_stats->vc_misses++;
		(rw == READ) ? p_stats->read_misses_combined++ : p_stats->write_misses_combined++;
		VC_evict_block = evict_choice(VC.blocks, VC.blocks_per_set);
		if (VC_evict_block->dirty) {
			p_stats->write_backs++;
			p_stats->bytes_transferred += bytes_per_block;
		}
		p_stats->bytes_transferred += bytes_per_block; //copy new bytes from mem
		copy_block(L1_evict_block, VC_evict_block);
		VC_evict_block->time = cur_time; //this block needs correct FIFO timing
		overwrite_block_for_new_addr(L1_evict_block, addr);
		if (rw == WRITE)
			L1_evict_block->dirty = 1;
	}
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
