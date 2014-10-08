#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cachesim.h"

struct cache L1, VC;

// common attribute to both caches
uint64_t bytes_per_block;
uint64_t _b;

// Given a set defined by first_block_in_set and blocks_per_set set, return a
// pointer to the block that should be evicted. See update_time_on_access for
// more details. This function can return invalid blocks.
struct block * evict_choice(struct block * first_block_in_set, uint64_t blocks_per_set) {
	struct block * choice = first_block_in_set, * this;
	uint64_t i;
	for (i = 0; i < blocks_per_set; i++) {
		this = first_block_in_set + i;
		if (!this->valid)
			return this;
		if (this->time < choice->time)
			choice = this;
	}
	return choice;
}

void print_cache_info(struct cache * c) {
	printf("the %s cache\n", c->name);
	printf("============\n");
	printf("\tbytes: %" PRIu64 "\n", bytes_per_block * c->n_blocks);
	printf("\tbytes/block: %" PRIu64 "\n", bytes_per_block);
	printf("\tblocks: %" PRIu64 "\n",c->n_blocks );
	printf("\tblocks/set: %" PRIu64 "\n", c->blocks_per_set);
	printf("\tsets: %" PRIu64 "\n", N_SETS_FROM_CACHE(c));
	printf("\tvictim cache: %s\n", (c->victim_cache == NULL) ? "null" : c->victim_cache->name);
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

char is_addr_in_block(uint64_t addr, struct block * b) {
	return BLOCK_TAG_FROM_ADDR(addr) == b->tag;
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

	L1.name = L1_name;
	L1.n_blocks = N_BLOCKS(c,b);
	L1.blocks_per_set = BLOCKS_PER_SET(s);
	L1.n_prefetch_blocks = k;
	L1.update_time_on_access = 1;
	L1.victim_cache = v ? &VC : NULL;
	L1.blocks = calloc(L1.n_blocks, sizeof(struct block));
	print_cache_info(&L1);

	if (v) {
		VC.name = VC_name;
		VC.n_blocks = v;
		VC.blocks_per_set = v;
		VC.n_prefetch_blocks = 0;
		VC.update_time_on_access = 0;
		VC.victim_cache = NULL;
		VC.blocks = calloc(VC.n_blocks, sizeof(struct block));
		print_cache_info(&VC);
	}

	L1.blocks[1023].tag = BLOCK_TAG_FROM_ADDR(0x0);
	print_block_info(&L1.blocks[1023]);
	printf("%s\n", is_addr_in_block(31, &L1.blocks[1023]) ? "yeap": "nope");
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
	p_stats->accesses++;

	if (rw == READ) {
		p_stats->reads++;
	} else {
		p_stats->writes++;
	}
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(struct cache_stats_t *p_stats) {
	free(L1.blocks);
	free(VC.blocks);
}
