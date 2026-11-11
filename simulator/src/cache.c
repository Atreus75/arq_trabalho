#include "cache.h"
#include <string.h>

static void update_rates(Mic1Cache *cache) {
    uint64_t total = cache->stats.totalHits + cache->stats.totalMisses;
    if (total > 0) {
        cache->stats.hitRate = (double)cache->stats.totalHits / (double)total;
        cache->stats.missRate = (double)cache->stats.totalMisses / (double)total;
    } else {
        cache->stats.hitRate = 0.0;
        cache->stats.missRate = 0.0;
    }
}

static void decode_address(const Mic1Cache *cache, uint16_t address,
                           int *offset, uint16_t *blockAddress,
                           int *setIndex, uint16_t *tag) {
    int blockSize = cache->config.blockSizeWords;
    if (blockSize <= 0) blockSize = 1;
    *offset = (int)(address % (unsigned)blockSize);
    *blockAddress = (uint16_t)(address - (unsigned)*offset);
    uint32_t blockNum = (uint32_t)(*blockAddress / (unsigned)blockSize);

    if (cache->config.mapping == CACHE_MAP_FULLY_ASSOCIATIVE) {
        *setIndex = 0;
        *tag = (uint16_t)blockNum;
    } else {
        int sets = cache->numSets > 0 ? cache->numSets : 1;
        *setIndex = (int)(blockNum % (unsigned)sets);
        *tag = (uint16_t)(blockNum / (unsigned)sets);
    }
}

static int select_victim_way(const Mic1Cache *cache, int setIndex) {
    int ways = cache->config.numWays > 0 ? cache->config.numWays : 1;
    int base = setIndex * ways;

    /* 1. Primeiro verifica se existe alguma via vazia (invalida) */
    for (int w = 0; w < ways; w++) {
        if (!cache->lines[base + w].valid) {
            return w;
        }
    }

    /* 2. Todas as vias estao ocupadas: aplica politica de substituicao */
    uint64_t oldest = UINT64_MAX;
    int victim = 0;
    for (int w = 0; w < ways; w++) {
        uint64_t t = (cache->config.replacement == CACHE_REPLACE_LRU)
                     ? cache->lines[base + w].lastAccessTime
                     : cache->lines[base + w].loadTime;
        if (t < oldest) {
            oldest = t;
            victim = w;
        }
    }
    return victim;
}

static void evict_line_if_dirty(Mic1Cache *cache, int lineIdx, uint16_t *mainMemory) {
    Mic1CacheLine *l = &cache->lines[lineIdx];
    if (l->valid && l->dirty && mainMemory) {
        int blockSize = cache->config.blockSizeWords;
        for (int i = 0; i < blockSize; i++) {
            uint16_t target = (uint16_t)((l->blockAddress + (unsigned)i) & 0x0fff);
            mainMemory[target] = l->words[i];
        }
        l->dirty = false;
    }
}

void cache_init_default(Mic1Cache *cache) {
    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
    cache->config.enabled = true;
    cache->config.mapping = CACHE_MAP_DIRECT;
    cache->config.replacement = CACHE_REPLACE_LRU;
    cache->config.writePolicy = CACHE_WRITE_THROUGH;
    cache->config.capacityWords = 16;
    cache->config.blockSizeWords = 2;
    cache->config.numWays = 1;
    cache->config.penaltyCycles = 4;
    cache->numLinesTotal = 16 / 2; // 8 linhas
    cache->numSets = 8;
}

void cache_reset(Mic1Cache *cache) {
    if (!cache) return;
    Mic1CacheConfig cfg = cache->config;
    int sets = cache->numSets;
    int total = cache->numLinesTotal;
    memset(cache, 0, sizeof(*cache));
    cache->config = cfg;
    cache->numSets = sets;
    cache->numLinesTotal = total;
}

void cache_flush(Mic1Cache *cache, uint16_t *mainMemory) {
    if (!cache || !mainMemory) return;
    for (int i = 0; i < cache->numLinesTotal; i++) {
        evict_line_if_dirty(cache, i, mainMemory);
    }
}

static bool is_power_of_two(int x) {
    return x > 0 && (x & (x - 1)) == 0;
}

bool cache_configure(Mic1Cache *cache, const Mic1CacheConfig *cfg, uint16_t *mainMemory) {
    if (!cache || !cfg) return false;

    int cap = cfg->capacityWords;
    int blk = cfg->blockSizeWords;
    if (cap < 8 || cap > 64 || !is_power_of_two(cap)) return false;
    if (blk < 1 || blk > 8 || !is_power_of_two(blk) || blk > cap) return false;

    int totalLines = cap / blk;
    if (totalLines > MIC1_CACHE_MAX_LINES) return false;

    int numWays = 1;
    int numSets = totalLines;

    if (cfg->mapping == CACHE_MAP_DIRECT) {
        numWays = 1;
        numSets = totalLines;
    } else if (cfg->mapping == CACHE_MAP_FULLY_ASSOCIATIVE) {
        numSets = 1;
        numWays = totalLines;
    } else if (cfg->mapping == CACHE_MAP_SET_ASSOCIATIVE) {
        numWays = cfg->numWays;
        if (numWays <= 1 || numWays > totalLines || (totalLines % numWays) != 0) {
            return false;
        }
        numSets = totalLines / numWays;
    } else {
        return false;
    }

    if (cfg->replacement != CACHE_REPLACE_LRU && cfg->replacement != CACHE_REPLACE_FIFO) {
        return false;
    }
    if (cfg->writePolicy != CACHE_WRITE_THROUGH && cfg->writePolicy != CACHE_WRITE_BACK) {
        return false;
    }
    if (cfg->penaltyCycles < 0) return false;

    /* Descarrega dados sujos antes de reconfigurar a geometria */
    cache_flush(cache, mainMemory);

    cache->config = *cfg;
    cache->config.numWays = numWays;
    if (cache->config.penaltyCycles == 0) {
        cache->config.penaltyCycles = 4;
    }
    cache->numLinesTotal = totalLines;
    cache->numSets = numSets;
    cache_reset(cache);

    return true;
}

uint16_t cache_read(Mic1Cache *cache, uint16_t address, const uint16_t *mainMemory, bool *outHit) {
    address &= 0x0fff;
    if (!cache) {
        if (outHit) *outHit = false;
        return mainMemory ? mainMemory[address] : 0;
    }

    if (!cache->config.enabled) {
        if (outHit) *outHit = false;
        cache->lastAccess.hasAccess = false;
        return mainMemory ? mainMemory[address] : 0;
    }

    int offset = 0, setIndex = 0;
    uint16_t blockAddress = 0, tag = 0;
    decode_address(cache, address, &offset, &blockAddress, &setIndex, &tag);

    int ways = cache->config.numWays > 0 ? cache->config.numWays : 1;
    int base = setIndex * ways;
    int hitWay = -1;

    for (int w = 0; w < ways; w++) {
        Mic1CacheLine *l = &cache->lines[base + w];
        if (l->valid && l->tag == tag) {
            hitWay = w;
            break;
        }
    }

    if (hitWay >= 0) {
        /* Read HIT */
        Mic1CacheLine *hitLine = &cache->lines[base + hitWay];
        cache->stats.readHits++;
        cache->stats.totalHits++;
        hitLine->lastAccessTime = ++cache->accessCounter;
        if (outHit) *outHit = true;

        cache->lastAccess.hasAccess = true;
        strncpy(cache->lastAccess.event, "READ", sizeof(cache->lastAccess.event));
        cache->lastAccess.address = address;
        cache->lastAccess.block = blockAddress;
        cache->lastAccess.setIndex = setIndex;
        cache->lastAccess.wayIndex = hitWay;
        cache->lastAccess.tag = tag;
        cache->lastAccess.hit = true;

        update_rates(cache);
        return hitLine->words[offset];
    }

    /* Read MISS */
    cache->stats.readMisses++;
    cache->stats.totalMisses++;
    cache->stats.penaltyCyclesTotal += (uint64_t)cache->config.penaltyCycles;
    if (outHit) *outHit = false;

    int victimWay = select_victim_way(cache, setIndex);
    int lineIdx = base + victimWay;
    evict_line_if_dirty(cache, lineIdx, (uint16_t *)(uintptr_t)mainMemory);

    Mic1CacheLine *victim = &cache->lines[lineIdx];
    victim->valid = true;
    victim->dirty = false;
    victim->tag = tag;
    victim->blockAddress = blockAddress;

    int blockSize = cache->config.blockSizeWords;
    for (int i = 0; i < blockSize; i++) {
        uint16_t a = (uint16_t)((blockAddress + (unsigned)i) & 0x0fff);
        victim->words[i] = mainMemory ? mainMemory[a] : 0;
    }
    victim->loadTime = ++cache->accessCounter;
    victim->lastAccessTime = cache->accessCounter;

    cache->lastAccess.hasAccess = true;
    strncpy(cache->lastAccess.event, "READ", sizeof(cache->lastAccess.event));
    cache->lastAccess.address = address;
    cache->lastAccess.block = blockAddress;
    cache->lastAccess.setIndex = setIndex;
    cache->lastAccess.wayIndex = victimWay;
    cache->lastAccess.tag = tag;
    cache->lastAccess.hit = false;

    update_rates(cache);
    return victim->words[offset];
}

void cache_write(Mic1Cache *cache, uint16_t address, uint16_t value, uint16_t *mainMemory, bool *outHit) {
    address &= 0x0fff;
    if (!cache) {
        if (outHit) *outHit = false;
        if (mainMemory) mainMemory[address] = value;
        return;
    }

    if (!cache->config.enabled) {
        if (outHit) *outHit = false;
        cache->lastAccess.hasAccess = false;
        if (mainMemory) mainMemory[address] = value;
        return;
    }

    int offset = 0, setIndex = 0;
    uint16_t blockAddress = 0, tag = 0;
    decode_address(cache, address, &offset, &blockAddress, &setIndex, &tag);

    int ways = cache->config.numWays > 0 ? cache->config.numWays : 1;
    int base = setIndex * ways;
    int hitWay = -1;

    for (int w = 0; w < ways; w++) {
        Mic1CacheLine *l = &cache->lines[base + w];
        if (l->valid && l->tag == tag) {
            hitWay = w;
            break;
        }
    }

    if (hitWay >= 0) {
        /* Write HIT */
        Mic1CacheLine *hitLine = &cache->lines[base + hitWay];
        cache->stats.writeHits++;
        cache->stats.totalHits++;
        hitLine->lastAccessTime = ++cache->accessCounter;
        hitLine->words[offset] = value;
        if (outHit) *outHit = true;

        if (cache->config.writePolicy == CACHE_WRITE_THROUGH) {
            if (mainMemory) mainMemory[address] = value;
            hitLine->dirty = false;
        } else {
            hitLine->dirty = true;
        }

        cache->lastAccess.hasAccess = true;
        strncpy(cache->lastAccess.event, "WRITE", sizeof(cache->lastAccess.event));
        cache->lastAccess.address = address;
        cache->lastAccess.block = blockAddress;
        cache->lastAccess.setIndex = setIndex;
        cache->lastAccess.wayIndex = hitWay;
        cache->lastAccess.tag = tag;
        cache->lastAccess.hit = true;

        update_rates(cache);
        return;
    }

    /* Write MISS */
    cache->stats.writeMisses++;
    cache->stats.totalMisses++;
    cache->stats.penaltyCyclesTotal += (uint64_t)cache->config.penaltyCycles;
    if (outHit) *outHit = false;

    if (cache->config.writePolicy == CACHE_WRITE_THROUGH) {
        /* No-Write-Allocate: escreve direto na memoria principal */
        if (mainMemory) mainMemory[address] = value;

        cache->lastAccess.hasAccess = true;
        strncpy(cache->lastAccess.event, "WRITE", sizeof(cache->lastAccess.event));
        cache->lastAccess.address = address;
        cache->lastAccess.block = blockAddress;
        cache->lastAccess.setIndex = setIndex;
        cache->lastAccess.wayIndex = -1;
        cache->lastAccess.tag = tag;
        cache->lastAccess.hit = false;

        update_rates(cache);
        return;
    } else {
        /* Write-Allocate: aloca bloco na cache, atualiza palavra e marca dirty */
        int victimWay = select_victim_way(cache, setIndex);
        int lineIdx = base + victimWay;
        evict_line_if_dirty(cache, lineIdx, mainMemory);

        Mic1CacheLine *victim = &cache->lines[lineIdx];
        victim->valid = true;
        victim->tag = tag;
        victim->blockAddress = blockAddress;

        int blockSize = cache->config.blockSizeWords;
        for (int i = 0; i < blockSize; i++) {
            uint16_t a = (uint16_t)((blockAddress + (unsigned)i) & 0x0fff);
            victim->words[i] = mainMemory ? mainMemory[a] : 0;
        }
        victim->words[offset] = value;
        victim->dirty = true;
        victim->loadTime = ++cache->accessCounter;
        victim->lastAccessTime = cache->accessCounter;

        cache->lastAccess.hasAccess = true;
        strncpy(cache->lastAccess.event, "WRITE", sizeof(cache->lastAccess.event));
        cache->lastAccess.address = address;
        cache->lastAccess.block = blockAddress;
        cache->lastAccess.setIndex = setIndex;
        cache->lastAccess.wayIndex = victimWay;
        cache->lastAccess.tag = tag;
        cache->lastAccess.hit = false;

        update_rates(cache);
        return;
    }
}
