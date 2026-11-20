#include "cache.h"
#include <string.h>

static void update_rates(Mic1Cache *cache) {
    uint64_t total = cache->stats.totalHits + cache->stats.totalMisses;
    if (total > 0) {
        cache->stats.taxaHit = (double)cache->stats.totalHits / (double)total;
        cache->stats.taxaMiss = (double)cache->stats.totalMisses / (double)total;
    } else {
        cache->stats.taxaHit = 0.0;
        cache->stats.taxaMiss = 0.0;
    }
}

static void decode_address(const Mic1Cache *cache, uint16_t endereco,
                           int *desloc, uint16_t *endBloco,
                           int *conjunto, uint16_t *tag) {
    int tamBloco = cache->config.tamBloco;
    if (tamBloco <= 0) tamBloco = 1;
    *desloc = (int)(endereco % (unsigned)tamBloco);
    *endBloco = (uint16_t)(endereco - (unsigned)*desloc);
    uint32_t numBloco = (uint32_t)(*endBloco / (unsigned)tamBloco);

    if (cache->config.mapeamento == CACHE_MAP_FULLY_ASSOCIATIVE) {
        *conjunto = 0;
        *tag = (uint16_t)numBloco;
    } else {
        int conjuntos = cache->numConjuntos > 0 ? cache->numConjuntos : 1;
        *conjunto = (int)(numBloco % (unsigned)conjuntos);
        *tag = (uint16_t)(numBloco / (unsigned)conjuntos);
    }
}

static int select_victim_way(const Mic1Cache *cache, int conjunto) {
    int vias = cache->config.numVias > 0 ? cache->config.numVias : 1;
    int base = conjunto * vias;

    // procura via vazia
    for (int w = 0; w < vias; w++) {
        if (!cache->linhas[base + w].valido) {
            return w;
        }
    }

    // todas ocupadas: acha a mais antiga
    uint64_t maisAntigo = UINT64_MAX;
    int vitima = 0;
    for (int w = 0; w < vias; w++) {
        uint64_t t = (cache->config.substituicao == CACHE_REPLACE_LRU)
                     ? cache->linhas[base + w].ultimoAcesso
                     : cache->linhas[base + w].tempoCarga;
        if (t < maisAntigo) {
            maisAntigo = t;
            vitima = w;
        }
    }
    return vitima;
}

static void evict_line_if_dirty(Mic1Cache *cache, int idxLinha, uint16_t *memPrincipal) {
    Mic1CacheLine *l = &cache->linhas[idxLinha];
    if (l->valido && l->sujo && memPrincipal) {
        int tamBloco = cache->config.tamBloco;
        for (int i = 0; i < tamBloco; i++) {
            uint16_t alvo = (uint16_t)((l->endBloco + (unsigned)i) & 0x0fff);
            memPrincipal[alvo] = l->palavras[i];
        }
        l->sujo = false;
    }
}

void cache_init_default(Mic1Cache *cache) {
    if (!cache) return;
    memset(cache, 0, sizeof(*cache));
    cache->config.ativa = true;
    cache->config.mapeamento = CACHE_MAP_DIRECT;
    cache->config.substituicao = CACHE_REPLACE_LRU;
    cache->config.politicaEscrita = CACHE_WRITE_THROUGH;
    cache->config.capacidade = 16;
    cache->config.tamBloco = 2;
    cache->config.numVias = 1;
    cache->config.ciclosPenalidade = 4;
    cache->totalLinhas = 8;
    cache->numConjuntos = 8;
}

void cache_reset(Mic1Cache *cache) {
    if (!cache) return;
    Mic1CacheConfig cfg = cache->config;
    int conjuntos = cache->numConjuntos;
    int total = cache->totalLinhas;
    memset(cache, 0, sizeof(*cache));
    cache->config = cfg;
    cache->numConjuntos = conjuntos;
    cache->totalLinhas = total;
}

void cache_flush(Mic1Cache *cache, uint16_t *memPrincipal) {
    if (!cache || !memPrincipal) return;
    for (int i = 0; i < cache->totalLinhas; i++) {
        evict_line_if_dirty(cache, i, memPrincipal);
    }
}

static bool is_power_of_two(int x) {
    return x > 0 && (x & (x - 1)) == 0;
}

bool cache_configure(Mic1Cache *cache, const Mic1CacheConfig *cfg, uint16_t *memPrincipal) {
    if (!cache || !cfg) return false;

    int cap = cfg->capacidade;
    int bloco = cfg->tamBloco;
    if (cap < 8 || cap > 64 || !is_power_of_two(cap)) return false;
    if (bloco < 1 || bloco > 8 || !is_power_of_two(bloco) || bloco > cap) return false;

    int totalLinhas = cap / bloco;
    if (totalLinhas > MIC1_CACHE_MAX_LINES) return false;

    int numVias = 1;
    int numConjuntos = totalLinhas;

    if (cfg->mapeamento == CACHE_MAP_DIRECT) {
        numVias = 1;
        numConjuntos = totalLinhas;
    } else if (cfg->mapeamento == CACHE_MAP_FULLY_ASSOCIATIVE) {
        numConjuntos = 1;
        numVias = totalLinhas;
    } else if (cfg->mapeamento == CACHE_MAP_SET_ASSOCIATIVE) {
        numVias = cfg->numVias;
        if (numVias <= 1 || numVias > totalLinhas || (totalLinhas % numVias) != 0) {
            return false;
        }
        numConjuntos = totalLinhas / numVias;
    } else {
        return false;
    }

    if (cfg->substituicao != CACHE_REPLACE_LRU && cfg->substituicao != CACHE_REPLACE_FIFO) {
        return false;
    }
    if (cfg->politicaEscrita != CACHE_WRITE_THROUGH && cfg->politicaEscrita != CACHE_WRITE_BACK) {
        return false;
    }
    if (cfg->ciclosPenalidade < 0) return false;

    /* Descarrega dados sujos antes de reconfigurar a geometria */
    cache_flush(cache, memPrincipal);

    cache->config = *cfg;
    cache->config.numVias = numVias;
    if (cache->config.ciclosPenalidade == 0) {
        cache->config.ciclosPenalidade = 4;
    }
    cache->totalLinhas = totalLinhas;
    cache->numConjuntos = numConjuntos;
    cache_reset(cache);

    return true;
}

uint16_t cache_read(Mic1Cache *cache, uint16_t endereco, const uint16_t *memPrincipal, bool *teveHit) {
    endereco &= 0x0fff;
    if (!cache) {
        if (teveHit) *teveHit = false;
        return memPrincipal ? memPrincipal[endereco] : 0;
    }

    if (!cache->config.ativa) {
        if (teveHit) *teveHit = false;
        cache->ultimoAcesso.teveAcesso = false;
        return memPrincipal ? memPrincipal[endereco] : 0;
    }

    int desloc = 0, conjunto = 0;
    uint16_t endBloco = 0, tag = 0;
    decode_address(cache, endereco, &desloc, &endBloco, &conjunto, &tag);

    int vias = cache->config.numVias > 0 ? cache->config.numVias : 1;
    int base = conjunto * vias;
    int viaHit = -1;

    for (int w = 0; w < vias; w++) {
        Mic1CacheLine *l = &cache->linhas[base + w];
        if (l->valido && l->tag == tag) {
            viaHit = w;
            break;
        }
    }

    if (viaHit >= 0) {
        Mic1CacheLine *linhaHit = &cache->linhas[base + viaHit];
        cache->stats.leiturasHit++;
        cache->stats.totalHits++;
        linhaHit->ultimoAcesso = ++cache->contAcessos;
        if (teveHit) *teveHit = true;

        cache->ultimoAcesso.teveAcesso = true;
        strncpy(cache->ultimoAcesso.evento, "READ", sizeof(cache->ultimoAcesso.evento));
        cache->ultimoAcesso.endereco = endereco;
        cache->ultimoAcesso.bloco = endBloco;
        cache->ultimoAcesso.conjunto = conjunto;
        cache->ultimoAcesso.via = viaHit;
        cache->ultimoAcesso.tag = tag;
        cache->ultimoAcesso.hit = true;

        update_rates(cache);
        return linhaHit->palavras[desloc];
    }

    cache->stats.leiturasMiss++;
    cache->stats.totalMisses++;
    cache->stats.totalPenalidade += (uint64_t)cache->config.ciclosPenalidade;
    if (teveHit) *teveHit = false;

    int viaVitima = select_victim_way(cache, conjunto);
    int idxLinha = base + viaVitima;
    evict_line_if_dirty(cache, idxLinha, (uint16_t *)memPrincipal);

    Mic1CacheLine *vitima = &cache->linhas[idxLinha];
    vitima->valido = true;
    vitima->sujo = false;
    vitima->tag = tag;
    vitima->endBloco = endBloco;

    int tamBloco = cache->config.tamBloco;
    for (int i = 0; i < tamBloco; i++) {
        uint16_t a = (uint16_t)((endBloco + (unsigned)i) & 0x0fff);
        vitima->palavras[i] = memPrincipal ? memPrincipal[a] : 0;
    }
    vitima->tempoCarga = ++cache->contAcessos;
    vitima->ultimoAcesso = cache->contAcessos;

    cache->ultimoAcesso.teveAcesso = true;
    strncpy(cache->ultimoAcesso.evento, "READ", sizeof(cache->ultimoAcesso.evento));
    cache->ultimoAcesso.endereco = endereco;
    cache->ultimoAcesso.bloco = endBloco;
    cache->ultimoAcesso.conjunto = conjunto;
    cache->ultimoAcesso.via = viaVitima;
    cache->ultimoAcesso.tag = tag;
    cache->ultimoAcesso.hit = false;

    update_rates(cache);
    return vitima->palavras[desloc];
}

void cache_write(Mic1Cache *cache, uint16_t endereco, uint16_t valor, uint16_t *memPrincipal, bool *teveHit) {
    endereco &= 0x0fff;
    if (!cache) {
        if (teveHit) *teveHit = false;
        if (memPrincipal) memPrincipal[endereco] = valor;
        return;
    }

    if (!cache->config.ativa) {
        if (teveHit) *teveHit = false;
        cache->ultimoAcesso.teveAcesso = false;
        if (memPrincipal) memPrincipal[endereco] = valor;
        return;
    }

    int desloc = 0, conjunto = 0;
    uint16_t endBloco = 0, tag = 0;
    decode_address(cache, endereco, &desloc, &endBloco, &conjunto, &tag);

    int vias = cache->config.numVias > 0 ? cache->config.numVias : 1;
    int base = conjunto * vias;
    int viaHit = -1;

    for (int w = 0; w < vias; w++) {
        Mic1CacheLine *l = &cache->linhas[base + w];
        if (l->valido && l->tag == tag) {
            viaHit = w;
            break;
        }
    }

    if (viaHit >= 0) {
        Mic1CacheLine *linhaHit = &cache->linhas[base + viaHit];
        cache->stats.escritasHit++;
        cache->stats.totalHits++;
        linhaHit->ultimoAcesso = ++cache->contAcessos;
        linhaHit->palavras[desloc] = valor;
        if (teveHit) *teveHit = true;

        if (cache->config.politicaEscrita == CACHE_WRITE_THROUGH) {
            if (memPrincipal) memPrincipal[endereco] = valor;
            linhaHit->sujo = false;
        } else {
            linhaHit->sujo = true;
        }

        cache->ultimoAcesso.teveAcesso = true;
        strncpy(cache->ultimoAcesso.evento, "WRITE", sizeof(cache->ultimoAcesso.evento));
        cache->ultimoAcesso.endereco = endereco;
        cache->ultimoAcesso.bloco = endBloco;
        cache->ultimoAcesso.conjunto = conjunto;
        cache->ultimoAcesso.via = viaHit;
        cache->ultimoAcesso.tag = tag;
        cache->ultimoAcesso.hit = true;

        update_rates(cache);
        return;
    }

    /* Write MISS */
    cache->stats.escritasMiss++;
    cache->stats.totalMisses++;
    cache->stats.totalPenalidade += (uint64_t)cache->config.ciclosPenalidade;
    if (teveHit) *teveHit = false;

    if (cache->config.politicaEscrita == CACHE_WRITE_THROUGH) {
        /* No-Write-Allocate: escreve direto na memoria principal */
        if (memPrincipal) memPrincipal[endereco] = valor;

        cache->ultimoAcesso.teveAcesso = true;
        strncpy(cache->ultimoAcesso.evento, "WRITE", sizeof(cache->ultimoAcesso.evento));
        cache->ultimoAcesso.endereco = endereco;
        cache->ultimoAcesso.bloco = endBloco;
        cache->ultimoAcesso.conjunto = conjunto;
        cache->ultimoAcesso.via = -1;
        cache->ultimoAcesso.tag = tag;
        cache->ultimoAcesso.hit = false;

        update_rates(cache);
        return;
    } else {
        /* Write-Allocate: aloca bloco na cache, atualiza palavra e marca dirty */
        int viaVitima = select_victim_way(cache, conjunto);
        int idxLinha = base + viaVitima;
        evict_line_if_dirty(cache, idxLinha, memPrincipal);

        Mic1CacheLine *vitima = &cache->linhas[idxLinha];
        vitima->valido = true;
        vitima->tag = tag;
        vitima->endBloco = endBloco;

        int tamBloco = cache->config.tamBloco;
        for (int i = 0; i < tamBloco; i++) {
            uint16_t a = (uint16_t)((endBloco + (unsigned)i) & 0x0fff);
            vitima->palavras[i] = memPrincipal ? memPrincipal[a] : 0;
        }
        vitima->palavras[desloc] = valor;
        vitima->sujo = true;
        vitima->tempoCarga = ++cache->contAcessos;
        vitima->ultimoAcesso = cache->contAcessos;

        cache->ultimoAcesso.teveAcesso = true;
        strncpy(cache->ultimoAcesso.evento, "WRITE", sizeof(cache->ultimoAcesso.evento));
        cache->ultimoAcesso.endereco = endereco;
        cache->ultimoAcesso.bloco = endBloco;
        cache->ultimoAcesso.conjunto = conjunto;
        cache->ultimoAcesso.via = viaVitima;
        cache->ultimoAcesso.tag = tag;
        cache->ultimoAcesso.hit = false;

        update_rates(cache);
        return;
    }
}
