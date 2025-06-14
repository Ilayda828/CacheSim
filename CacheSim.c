#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// A cache block containing valid, tag, time and data information
typedef struct {
    int valid;
    uint32_t tag;
    int time;
    char data[256];
} Block;

/* Struct :
s: number of sets (2^s), E: number of blocks (associativity), b: block size (2^b), blocks: [set][block] array */
typedef struct {
    int s, E, b;
    Block **blocks;
} Cache;

// The struct structure that holds the statistics
typedef struct {
    int hits;
    int misses;
    int evictions;
} Stats;

// Reads the block at the given address from the RAM.dat file and copies it to the buffer.
void read_from_ram(uint32_t addr, int block_size, char *buffer) {
    FILE *ram = fopen("RAM.dat", "rb");
    if (!ram) {
        perror("RAM.dat open failed");
        exit(1);
    }

    uint32_t aligned_addr = addr & ~(block_size - 1);
    fseek(ram, aligned_addr, SEEK_SET);
    fread(buffer, 1, block_size, ram);
    fclose(ram);
}

// Creates an empty and clean cache according to the parameters.
Cache* init_cache(int s, int E, int b) {
    Cache *cache = malloc(sizeof(Cache));
    cache->s = s;
    cache->E = E;
    cache->b = b;
    int sets = 1 << s;

    cache->blocks = malloc(sets * sizeof(Block*));
    for (int i = 0; i < sets; i++) {
        cache->blocks[i] = malloc(E * sizeof(Block));
        for (int j = 0; j < E; j++) {
            cache->blocks[i][j].valid = 0;
            cache->blocks[i][j].tag = 0;
            cache->blocks[i][j].time = 0;
            memset(cache->blocks[i][j].data, 0, sizeof(cache->blocks[i][j].data));
        }
    }

    return cache;
}

// Extracts the tag value from the given address.
uint32_t get_tag(uint32_t addr, int s, int b) {
    return addr >> (s + b);
}

// Extracts the set index from the given address.
uint32_t get_set(uint32_t addr, int s, int b) {
    return (addr >> b) & ((1 << s) - 1);
}

// Searches the address in the cache, returns a hit or miss status, and updates the LRU time.
int access_cache(Cache *cache, uint32_t addr, int is_store, Stats *stat, int global_time, const char *cache_name, int update_lru) {
    uint32_t set = get_set(addr, cache->s, cache->b);
    uint32_t tag = get_tag(addr, cache->s, cache->b);
    Block *blocks = cache->blocks[set];

    for (int i = 0; i < cache->E; i++) {
        if (blocks[i].valid && blocks[i].tag == tag) {
            stat->hits++;
            if (update_lru) {
                blocks[i].time = global_time;
            }
            printf("  %s hit\n", cache_name);
            return 1;
        }
    }

    stat->misses++;
    printf("  %s miss\n", cache_name);
    return 0;
}

// Brings the block at the address from RAM to the cache, evicts the block with LRU if necessary.
void bring_to_cache(Cache *cache, uint32_t addr, Stats *stat, int global_time, const char *cache_name) {
    uint32_t set = get_set(addr, cache->s, cache->b);
    uint32_t tag = get_tag(addr, cache->s, cache->b);
    Block *blocks = cache->blocks[set];

    int evict_index = -1;
    for (int i = 0; i < cache->E; i++) {
        if (!blocks[i].valid) {
            evict_index = i;
            break;
        }
    }

    if (evict_index == -1) {
        stat->evictions++;
        evict_index = 0;
        int min_time = blocks[0].time;
        for (int i = 1; i < cache->E; i++) {
            if (blocks[i].time < min_time) {
                min_time = blocks[i].time;
                evict_index = i;
            }
        }
    }

    blocks[evict_index].tag = tag;
    blocks[evict_index].valid = 1;
    blocks[evict_index].time = global_time;
    read_from_ram(addr, 1 << cache->b, blocks[evict_index].data);

    printf("  Place in %s set %u\n", cache_name, set);
}

// Performs a store operation, searches the caches, or writes directly to RAM (write-no-allocate).
void handle_store(Cache *L1D, Cache *L2, uint32_t addr, Stats *s_L1D, Stats *s_L2, int time, const char *data) {
    int L1D_hit = access_cache(L1D, addr, 1, s_L1D, time, "L1D", 1);
    int L2_hit = access_cache(L2, addr, 1, s_L2, time, "L2", 1);

    if (L1D_hit || L2_hit) {
        printf("  Store %s in RAM\n", data);
    } else {
        printf("  Store %s in RAM (no allocation)\n", data);
    }
}

// Writes the cache content to a file.
void write_cache_to_file(Cache *cache, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    int sets = 1 << cache->s;
    int block_size = 1 << cache->b;

    for (int i = 0; i < sets; i++) {
        fprintf(fp, "Set %d\n", i);
        for (int j = 0; j < cache->E; j++) {
            Block *b = &cache->blocks[i][j];
            if (b->valid) {
                fprintf(fp, "%08x %d %d ", b->tag, b->time, b->valid);
                for (int k = 0; k < block_size; k++) {
                    fprintf(fp, "%02x", (unsigned char)b->data[k]);
                }
                fprintf(fp, "\n");
            }
        }
    }

    fclose(fp);
}

// Reads command line arguments, initializes caches, processes the trace file, and prints the results.
int main(int argc, char *argv[]) {
    // Default L1 and L2 cache configurations
    int L1s = 1, L1E = 2, L1b = 4;
    int L2s = 2, L2E = 4, L2b = 4;
    char *tracefile = NULL;

    // Process command line arguments
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-L1s")) L1s = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L1E")) L1E = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L1b")) L1b = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L2s")) L2s = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L2E")) L2E = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-L2b")) L2b = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-t")) tracefile = argv[++i];
    }

    // If there is no trace file, show usage information
    if (!tracefile) {
        fprintf(stderr, "Usage: %s -L1s <s> -L1E <E> -L1b <b> -L2s <s> -L2E <E> -L2b <b> -t <tracefile>\n", argv[0]);
        return 1;
    }

    // Initialize L1D, L1I and L2 caches
    Cache *L1D = init_cache(L1s, L1E, L1b);
    Cache *L1I = init_cache(L1s, L1E, L1b);
    Cache *L2 = init_cache(L2s, L2E, L2b);

    // Statistics structures
    Stats s_L1D = {0}, s_L1I = {0}, s_L2 = {0};

    // Open trace file
    FILE *fp = fopen(tracefile, "r");
    if (!fp) {
        perror("File open failed");
        return 1;
    }

    char line[256];
    int time = 0;
    while (fgets(line, sizeof(line), fp)) {
        char op;
        uint32_t addr;
        int size;
        char data[256] = "";

        if (line[0] == '\n') continue;

        // Parse the line (I, L, S, M operations)
        if (sscanf(line, "%c %x,%d,%255s", &op, &addr, &size, data) < 3) {
            if (sscanf(line, "%c %x,%d", &op, &addr, &size) < 3) {
                continue;
            }
        }

        printf("\n%c %08x, %d", op, addr, size);
        if (data[0] != '\0') {
            printf(", %s", data);
        }
        printf("\n");

        // Route by action type
        switch (op) {
            case 'I': { // Instruction fetch
                if (!access_cache(L1I, addr, 0, &s_L1I, time, "L1I", 1)) {
                    if (!access_cache(L2, addr, 0, &s_L2, time, "L2", 1)) {
                        bring_to_cache(L2, addr, &s_L2, time, "L2");
                    }
                    bring_to_cache(L1I, addr, &s_L1I, time, "L1I");
                }
                break;
            }
            case 'L': { // Load
                if (!access_cache(L1D, addr, 0, &s_L1D, time, "L1D", 1)) {
                    if (!access_cache(L2, addr, 0, &s_L2, time, "L2", 1)) {
                        bring_to_cache(L2, addr, &s_L2, time, "L2");
                    }
                    bring_to_cache(L1D, addr, &s_L1D, time, "L1D");
                }
                break;
            }
            case 'S': {  // Store
                handle_store(L1D, L2, addr, &s_L1D, &s_L2, time, data);
                break;
            }
            case 'M': {  // Modify = Load + Store
                if (!access_cache(L1D, addr, 0, &s_L1D, time, "L1D", 1)) {
                    if (!access_cache(L2, addr, 0, &s_L2, time, "L2", 1)) {
                        bring_to_cache(L2, addr, &s_L2, time, "L2");
                    }
                    bring_to_cache(L1D, addr, &s_L1D, time, "L1D");
                }

                access_cache(L1D, addr, 1, &s_L1D, time, "L1D", 1);
                access_cache(L2, addr, 1, &s_L2, time, "L2", 1);
                printf("  Store %s in RAM\n", data);
                break;
            }
            default:
                break;
        }

        time++;
    }
    fclose(fp);// Close trace file

    // Print last statistics
    printf("\nL1I-hits:%d L1I-misses:%d L1I-evictions:%d\n", s_L1I.hits, s_L1I.misses, s_L1I.evictions);
    printf("L1D-hits:%d L1D-misses:%d L1D-evictions:%d\n", s_L1D.hits, s_L1D.misses, s_L1D.evictions);
    printf("L2-hits:%d L2-misses:%d L2-evictions:%d\n", s_L2.hits, s_L2.misses, s_L2.evictions);

    // Write cache contents to files
    write_cache_to_file(L1I, "L1I.txt");
    write_cache_to_file(L1D, "L1D.txt");
    write_cache_to_file(L2,  "L2.txt");

    // Clear memory (free)
    int L1_sets = 1 << L1s;
    for (int i = 0; i < L1_sets; i++) {
        free(L1D->blocks[i]);
        free(L1I->blocks[i]);
    }
    free(L1D->blocks);
    free(L1I->blocks);
    free(L1D);
    free(L1I);

    int L2_sets = 1 << L2s;
    for (int i = 0; i < L2_sets; i++) {
        free(L2->blocks[i]);
    }

    free(L2->blocks);
    free(L2);
    return 0;
}