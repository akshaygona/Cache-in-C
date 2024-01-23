////////////////////////////////////////////////////////////////////////////////
// Main File:        csim.c
// This File:        csim.c
// Other Files:      N/A
// Semester:         CS 354 Lecture 003 Fall 2023
// Instructor:       Mansi
//
// Author:           Akshay Gona
// Email:            gona@wisc.edu
// CS Login:         gona
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons: N/A
//
// Online sources: N/A
//
//////////////////////////// 80 columns wide ///////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2013,2019-2020
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission for Spring 2023
//
////////////////////////////////////////////////////////////////////////////////

/*
 * csim.c:
 * A cache simulator that can replay traces (from Valgrind) and output
 * statistics for the number of hits, misses, and evictions.
 * The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/******************************************************************************/
/* DO NOT MODIFY THESE VARIABLES **********************************************/

//Globals set by command line args.
int b = 0; //number of block (b) bits
int s = 0; //number of set (s) bits
int W = 0; //number of lines per set

//Globals derived from command line args.
int B; //block size in bytes: B = 2^b
int S; //number of sets: S = 2^s

//Global counters to track cache statistics in access_data().
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

//Global to control trace output
int verbosity = 0; //print trace if set
/******************************************************************************/


//Type mem_addr_t: Use when dealing with addresses or address masks.
typedef unsigned long long int mem_addr_t;

// Type cache_line_t: Use when dealing with cache lines.
//
// NOTE: we don't need to actually store any data because we are only simulating
// whether an access was a cache hit/miss, not actually using it as a cache.
//
// TODO - COMPLETE THIS TYPE
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    // TODO: Add a field as needed by your implementation for LRU tracking.
    int cnt; 
} cache_line_t;

//Type cache_set_t: Use when dealing with cache sets
//Note: Each set is a pointer to a heap array of one or more cache lines.
typedef cache_line_t* cache_set_t;

//Type cache_t: Use when dealing with the cache.
//Note: A cache is a pointer to a heap array of one or more sets.
typedef cache_set_t* cache_t;

// Create the cache we're simulating.
//Note: A cache is a pointer to a heap array of one or more cache sets.
cache_t cache;

/* TODO - COMPLETE THIS FUNCTION
 * generate_bitmask:
 * Generate a bitmask where the least-signficant m bits are 1 and all other bits
 * are 0. For example, generate_bitmask(3) would return 0x00000007.
 */
int generate_bitmask(int m) {
    return pow(2, m) - 1; // use pow function to generate bitmask
}

/* TODO - COMPLETE THIS FUNCTION
 * init_cache:
 * - Initialize the global variables S and B based on s and b.
 * - Allocates the data structure for a cache with S sets and W lines per set.
 * - Initializes all valid bits and tags with 0s.
 */
void init_cache() {
    S = (1 << s); // S = 2^s
    B = (1 << b); // B = 2^b
    cache = (cache_set_t*)malloc(sizeof(cache_set_t) * S); // allocate cache
    if (cache == NULL){ // check if cache is null
        printf("out of memory for sets");
        exit(1);
    }
    for (int i = 0; i < S; i++) { // allocate cache lines
        cache[i] = (cache_line_t*)malloc(sizeof(cache_line_t) * W); // allocate cache lines
        if (cache[i] == NULL) { // check if cache line is null
            printf("err mallocing cache line: %s\n", strerror(errno));
            exit(1);
        }
        for (int j = 0; j < W; j++) { // initialize cache lines
            cache[i][j].valid = 0; // set valid bit to 0
            cache[i][j].tag = 0; // set tag to 0
            cache[i][j].cnt = 0; // set cnt to 0
        }
    }

}


/* TODO - COMPLETE THIS FUNCTION
 * free_cache:
 * Frees all heap allocated memory used by the cache.
 */
void free_cache() {
    for (int i = 0; i < S; i++) { // free cache lines
        free(cache[i]);
    }
    free(cache); // free cache
}


/* TODO - COMPLETE THIS FUNCTION
 * access_data:
 * Simulates data access at given "addr" memory address in the cache.
 *
 * If already in cache, increment hit_cnt
 * If not in cache, cache it (set tag), increment miss_cnt
 * If a line is evicted, increment evict_cnt
 */
void access_data(mem_addr_t addr) {
    int set = (addr >> b) & generate_bitmask(s); // get set index
    mem_addr_t tag = addr >> (s + b); // get tag
    int hit = 0; // initialize hit to 0
    for (int i = 0; i < W; i++) { // iterate through cache lines
        if (cache[set][i].valid && cache[set][i].tag == tag) {
            hit = 1;
            if (verbosity){
                printf("hit "); // print hit if verbosity is set
            }
            hit_cnt++; // increment hit count
            cache[set][i].cnt = 0;
            break;
        }
    }
    if (!hit) { // if not hit
        if (verbosity){
            printf("miss "); // print miss if verbosity is set
        }
        miss_cnt++; // increment miss count
        int lru_index = 0; // initialize lru index to 0
        int max = cache[set][0].cnt; // initialize max to first cache line
        for (int i = 0; i < W; i++) { // iterate through cache lines
            if (cache[set][i].valid == 0) { // if cache line is invalid
                lru_index = i; // set lru index to i
                break;
            } else if (cache[set][i].cnt > max) { // if cache line is valid and cnt is greater than max
                max = cache[set][i].cnt; // set max to cnt
                lru_index = i;  // set lru index to i
            }
            cache[set][i].cnt++; // increment cnt
        }
        if (cache[set][lru_index].valid) {  // if cache line is valid
            if (verbosity){
                printf("eviction "); // print eviction if verbosity is set
            }
            evict_cnt++; // increment evict count
        }
        cache[set][lru_index].valid = 1;
        cache[set][lru_index].tag = tag;
        cache[set][lru_index].cnt = 0;
    }
}


/* 
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Reads the input trace file line by line.
 * Extracts the type of each memory access : L/S/M
 * TRANSLATE each "L" as a load i.e. 1 memory access
 * TRANSLATE each "S" as a store i.e. 1 memory access
 * TRANSLATE each "M" as a load followed by a store i.e. 2 memory accesses 
 */
void replay_trace(char* trace_fn) {
    char buf[1000];
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if (!trace_fp) {
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);

            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

            switch (buf[1]) {
                case 'L':
                case 'S':
                    access_data(addr);
                    break;
                case 'M':
                    access_data(addr);
                    access_data(addr);
                    break;
            }

            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}


/*
 * print_usage:
 * Print information on how to use csim to standard output.
 */
void print_usage(char* argv[]) {
    printf("Usage: %s [-hv] -s <num> -W <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of s bits for set index.\n");
    printf("  -W <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of b bits for block offsets.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -W 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -W 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}


/*
 * print_summary:
 * Prints a summary of the cache simulation statistics to a file.
 */
void print_summary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}


/*
 * main:
 * Main parses command line args, makes the cache, replays the memory accesses
 * free the cache and print the summary statistics.  
 */
int main(int argc, char* argv[]) {
    char* trace_file = NULL;
    char c;

    // Parse the command line arguments: -h, -v, -s, -W, -b, -t 
    while ((c = getopt(argc, argv, "s:W:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'W':
                W = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }

    //Make sure that all required command line args were specified.
    if (s == 0 || W == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    //Initialize cache.
    init_cache();

    //Replay the memory access trace.
    replay_trace(trace_file);

    //Free memory allocated for cache.
    free_cache();

    //Print the statistics to a file.
    //DO NOT REMOVE: This function must be called for test_csim to work.
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;
}

// Spring 202301
