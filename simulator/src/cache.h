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
    bool valido;
    bool sujo;
    uint16_t tag;
    uint16_t endBloco; // endereco base na memoria principal
    uint16_t palavras[MIC1_CACHE_MAX_BLOCK_WORDS]; // dados do bloco
    uint64_t ultimoAcesso; // contador logico para LRU
    uint64_t tempoCarga; // contador logico para FIFO
} Mic1CacheLine;

typedef struct {
    bool ativa;
    Mic1CacheMapping mapeamento;
    Mic1CacheReplacement substituicao;
    Mic1CacheWritePolicy politicaEscrita;
    int capacidade; // 8, 16, 32, 64 (padrao: 16)
    int tamBloco; // 1, 2, 4, 8 (padrao: 2)
    int numVias; // 1 para direto, 2 ou 4 para conjunto, N para totalmente associativo
    int ciclosPenalidade; // ciclos de penalidade por miss (padrao: 4)
} Mic1CacheConfig;

typedef struct {
    uint64_t leiturasHit;
    uint64_t leiturasMiss;
    uint64_t escritasHit;
    uint64_t escritasMiss;
    uint64_t totalHits;
    uint64_t totalMisses;
    double taxaHit;
    double taxaMiss;
    uint64_t totalPenalidade;
} Mic1CacheStats;

typedef struct {
    bool teveAcesso;
    char evento[8]; // "READ" ou "WRITE"
    uint16_t endereco;
    uint16_t bloco;
    int conjunto;
    int via;
    uint16_t tag;
    bool hit;
} Mic1CacheLastAccess;

typedef struct {
    Mic1CacheConfig config;
    Mic1CacheStats stats;
    Mic1CacheLastAccess ultimoAcesso;
    uint64_t contAcessos;
    int numConjuntos;
    int totalLinhas;
    Mic1CacheLine linhas[MIC1_CACHE_MAX_LINES];
} Mic1Cache;

void cache_init_default(Mic1Cache *cache);
void cache_reset(Mic1Cache *cache);
void cache_flush(Mic1Cache *cache, uint16_t *memPrincipal);
bool cache_configure(Mic1Cache *cache, const Mic1CacheConfig *cfg, uint16_t *memPrincipal);
uint16_t cache_read(Mic1Cache *cache, uint16_t endereco, const uint16_t *memPrincipal, bool *teveHit);
void cache_write(Mic1Cache *cache, uint16_t endereco, uint16_t valor, uint16_t *memPrincipal, bool *teveHit);

#endif /* MIC1_CACHE_H */
