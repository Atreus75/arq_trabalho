#ifndef MIC1_CACHE_H
#define MIC1_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MIC1_CACHE_MAX_BLOCK_WORDS 8
#define MIC1_CACHE_MAX_LINES 64

typedef enum {
    CACHE_MAP_DIRECT,
    CACHE_MAP_FULLY_ASSOCIATIVE,
    CACHE_MAP_SET_ASSOCIATIVE
} Mic1CacheMapping;

typedef enum {
    CACHE_REPLACE_LRU,
    CACHE_REPLACE_FIFO
} Mic1CacheReplacement;

typedef enum {
    CACHE_WRITE_THROUGH,
    CACHE_WRITE_BACK
} Mic1CacheWritePolicy;

typedef struct {
    bool valid;
    bool dirty;
    uint16_t tag;
    uint16_t blockAddress; // endereco base na memoria principal
    uint16_t words[MIC1_CACHE_MAX_BLOCK_WORDS]; // dados do bloco
    uint64_t lastAccessTime; // contador logico para LRU
    uint64_t loadTime; // contador logico para FIFO
} Mic1CacheLine;

typedef struct {
    bool enabled;
    Mic1CacheMapping mapping;
    Mic1CacheReplacement replacement;
    Mic1CacheWritePolicy writePolicy;
    int capacityWords; // 8, 16, 32, 64 (padrao: 16)
    int blockSizeWords; // 1, 2, 4, 8 (padrao: 2)
    int numWays; // 1 para direto, 2 ou 4 para conjunto, N para totalmente associativo
    int penaltyCycles; // ciclos de penalidade por miss (padrao: 4)
} Mic1CacheConfig;

typedef struct {
    uint64_t readHits;
    uint64_t readMisses;
    uint64_t writeHits;
    uint64_t writeMisses;
    uint64_t totalHits;
    uint64_t totalMisses;
    double hitRate;
    double missRate;
    uint64_t penaltyCyclesTotal;
} Mic1CacheStats;

typedef struct {
    bool hasAccess;
    char event[8]; // "READ" ou "WRITE"
    uint16_t address;
    uint16_t block;
    int setIndex;
    int wayIndex;
    uint16_t tag;
    bool hit;
} Mic1CacheLastAccess;

typedef struct {
    Mic1CacheConfig config;
    Mic1CacheStats stats;
    Mic1CacheLastAccess lastAccess;
    uint64_t accessCounter;
    int numSets;
    int numLinesTotal;
    Mic1CacheLine lines[MIC1_CACHE_MAX_LINES];
} Mic1Cache;

void cache_init_default(Mic1Cache *cache);
void cache_reset(Mic1Cache *cache);
void cache_flush(Mic1Cache *cache, uint16_t *mainMemory);
bool cache_configure(Mic1Cache *cache, const Mic1CacheConfig *cfg, uint16_t *mainMemory);
uint16_t cache_read(Mic1Cache *cache, uint16_t address, const uint16_t *mainMemory, bool *outHit);
void cache_write(Mic1Cache *cache, uint16_t address, uint16_t value, uint16_t *mainMemory, bool *outHit);

#endif /* MIC1_CACHE_H */
