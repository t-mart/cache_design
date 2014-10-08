#ifndef CACHESIM_H
#define CACHESIM_H

#include <inttypes.h>

struct cache_stats_t {
  uint64_t accesses;
  uint64_t reads;
  uint64_t read_misses;
  uint64_t read_misses_combined;
  uint64_t writes;
  uint64_t write_misses;
  uint64_t write_misses_combined;
  uint64_t misses;
  uint64_t write_backs;
  uint64_t vc_misses;
  uint64_t prefetched_blocks;
  uint64_t useful_prefetches;
  uint64_t bytes_transferred;

  uint64_t hit_time;
  uint64_t miss_penalty;
  double   miss_rate;
  double   avg_access_time;
};

void cache_access(char rw, uint64_t address, struct cache_stats_t* p_stats);
void setup_cache(uint64_t c, uint64_t b, uint64_t s, uint64_t v, uint64_t k);
void complete_cache(struct cache_stats_t *p_stats);

static const uint64_t DEFAULT_C = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_V = 4;    /* 4 victim blocks */
static const uint64_t DEFAULT_K = 2;	/* 2 prefetch distance */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

// to help with debugging
static const char *    L1_name = "L1";
static const char *    VC_name = "VC";

struct block {
	char dirty;
	char valid;
	uint64_t tag;
	int time;
};

struct cache {
	const char * name;
	struct block * blocks;
	uint64_t n_blocks;
	uint64_t blocks_per_set;

  // UBER IMPORTANT, YET SUBTLE. this is the difference between LRU and FIFO!!
  // * FIFO blocks get time set ONCE (when theyre pulled in from mem)
	// * LRU blocks get their time set EVERY ACCESS
	// This generalizes the eviction function: it simply needs to find the
	// block with the oldest time
	char update_time_on_access;

	uint64_t n_prefetch_blocks;
  struct cache * victim_cache;
};


#define N_BLOCKS(c,b)        (1<<((c)-(b)))
#define N_SETS(c,b,s)        (1<<((c)-((b)+(s))))
#define N_SETS_FROM_CACHE(c_ptr)        ((c_ptr)->n_blocks / (c_ptr)->blocks_per_set)
#define N_BYTES(c)           (1<<(c))

#define BYTES_PER_BLOCK(b)   (1<<(b))
#define BLOCKS_PER_SET(s)    (1<<(s))

#define BLOCK_TAG_FROM_ADDR(addr) ((addr) & ~(bytes_per_block - 1))
#define SET_INDEX(addr, n_sets) (((addr)>>_b) & (n_sets - 1))

void print_cache_info(struct cache * c);
void print_block_info(struct block * b);
struct block * evict_choice(struct block * first_block_in_set, uint64_t blocks_per_set);
void copy_block(struct block * from, struct block * to);
char is_addr_in_block(uint64_t addr, struct block * b);

#endif /* CACHESIM_H */
