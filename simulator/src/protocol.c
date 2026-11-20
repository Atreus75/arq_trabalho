#include "protocol.h"
#include "mic1.h"
#include "assembler.h"
#include "json.h"
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static Mic1CacheConfig configCachePadrao;
static bool configCacheIniciada = false;

char *protocol_error(const char *msg) {
    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w,"ok",false);
    jw_key_str(&w,"errorMessage",msg); jw_obj_end(&w); char *s=jw_take(&w); jw_free(&w); return s;
}

static void jw_key_double(JsonWriter *w, const char *chave, double val) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.4f", val);
    char *virgula = strchr(tmp, ',');
    if (virgula) *virgula = '.';
    jw_key(w, chave);
    size_t n = strlen(tmp);
    if (w->len + n + 1 > w->cap) {
        size_t novaCap = (w->cap + n + 1) * 2;
        char *novoBuf = realloc(w->buf, novaCap);
        if (novoBuf) { w->buf = novoBuf; w->cap = novaCap; }
    }
    memcpy(w->buf + w->len, tmp, n);
    w->len += n;
    w->buf[w->len] = 0;
    if (w->depth >= 0 && w->depth < 64) {
        w->wrote_item[w->depth] = 1;
    }
}

static void cache_json(JsonWriter *w, const Mic1Cache *cache) {
    jw_obj_start(w);
    jw_key_bool(w, "enabled", cache->config.ativa);

    const char *mapeamento = "direct";
    if (cache->config.mapeamento == CACHE_MAP_FULLY_ASSOCIATIVE) mapeamento = "fully";
    else if (cache->config.mapeamento == CACHE_MAP_SET_ASSOCIATIVE) mapeamento = "set";
    jw_key_str(w, "mapping", mapeamento);

    const char *substituicao = (cache->config.substituicao == CACHE_REPLACE_FIFO) ? "fifo" : "lru";
    jw_key_str(w, "replacement", substituicao);

    const char *politica = (cache->config.politicaEscrita == CACHE_WRITE_BACK) ? "write-back" : "write-through";
    jw_key_str(w, "writePolicy", politica);

    jw_key_int(w, "capacityWords", cache->config.capacidade);
    jw_key_int(w, "blockSizeWords", cache->config.tamBloco);
    jw_key_int(w, "numSets", cache->numConjuntos);
    jw_key_int(w, "numWays", cache->config.numVias);
    jw_key_int(w, "penaltyCycles", cache->config.ciclosPenalidade);

    /* stats */
    jw_key(w, "stats");
    jw_obj_start(w);
    jw_key_int(w, "readHits", (long long)cache->stats.leiturasHit);
    jw_key_int(w, "readMisses", (long long)cache->stats.leiturasMiss);
    jw_key_int(w, "writeHits", (long long)cache->stats.escritasHit);
    jw_key_int(w, "writeMisses", (long long)cache->stats.escritasMiss);
    jw_key_int(w, "totalHits", (long long)cache->stats.totalHits);
    jw_key_int(w, "totalMisses", (long long)cache->stats.totalMisses);
    jw_key_double(w, "hitRate", cache->stats.taxaHit);
    jw_key_double(w, "missRate", cache->stats.taxaMiss);
    jw_key_int(w, "penaltyCyclesTotal", (long long)cache->stats.totalPenalidade);
    jw_obj_end(w);

    /* ultimoAcesso */
    jw_key(w, "lastAccess");
    jw_obj_start(w);
    jw_key_bool(w, "hasAccess", cache->ultimoAcesso.teveAcesso);
    jw_key_str(w, "event", cache->ultimoAcesso.evento);
    jw_key_int(w, "address", cache->ultimoAcesso.endereco);
    jw_key_int(w, "block", cache->ultimoAcesso.bloco);
    jw_key_int(w, "setIndex", cache->ultimoAcesso.conjunto);
    jw_key_int(w, "wayIndex", cache->ultimoAcesso.via);
    jw_key_int(w, "tag", cache->ultimoAcesso.tag);
    jw_key_bool(w, "hit", cache->ultimoAcesso.hit);
    jw_obj_end(w);

    /* linhas */
    jw_key(w, "lines");
    jw_arr_start(w);
    int total = cache->totalLinhas;
    int vias = cache->config.numVias > 0 ? cache->config.numVias : 1;
    for (int i = 0; i < total; i++) {
        const Mic1CacheLine *l = &cache->linhas[i];
        jw_obj_start(w);
        jw_key_int(w, "index", i);
        jw_key_int(w, "set", i / vias);
        jw_key_int(w, "way", i % vias);
        jw_key_bool(w, "valid", l->valido);
        jw_key_bool(w, "dirty", l->sujo);
        jw_key_int(w, "tag", l->tag);
        jw_key_int(w, "blockAddress", l->endBloco);

        jw_key(w, "words");
        jw_arr_start(w);
        int tamBloco = cache->config.tamBloco;
        for (int b = 0; b < tamBloco; b++) {
            jw_int(w, l->palavras[b]);
        }
        jw_arr_end(w);

        jw_key_int(w, "lastAccessTime", (long long)l->ultimoAcesso);
        jw_key_int(w, "loadTime", (long long)l->tempoCarga);
        jw_obj_end(w);
    }
    jw_arr_end(w);

    jw_obj_end(w);
}

static void cache_from_json(Mic1Cache *cache, const JsonValue *valCache) {
    if (!valCache || valCache->type != JSON_OBJECT) return;

    cache->config.ativa = json_get_bool(valCache, "enabled", cache->config.ativa) != 0;

    const char *m = json_get_string(valCache, "mapping", "");
    if (!strcmp(m, "direct")) cache->config.mapeamento = CACHE_MAP_DIRECT;
    else if (!strcmp(m, "fully")) cache->config.mapeamento = CACHE_MAP_FULLY_ASSOCIATIVE;
    else if (!strcmp(m, "set")) cache->config.mapeamento = CACHE_MAP_SET_ASSOCIATIVE;

    const char *r = json_get_string(valCache, "replacement", "");
    if (!strcmp(r, "lru")) cache->config.substituicao = CACHE_REPLACE_LRU;
    else if (!strcmp(r, "fifo")) cache->config.substituicao = CACHE_REPLACE_FIFO;

    const char *p = json_get_string(valCache, "writePolicy", "");
    if (!strcmp(p, "write-through")) cache->config.politicaEscrita = CACHE_WRITE_THROUGH;
    else if (!strcmp(p, "write-back")) cache->config.politicaEscrita = CACHE_WRITE_BACK;

    cache->config.capacidade = (int)json_get_int(valCache, "capacityWords", cache->config.capacidade);
    cache->config.tamBloco = (int)json_get_int(valCache, "blockSizeWords", cache->config.tamBloco);
    cache->config.numVias = (int)json_get_int(valCache, "numWays", cache->config.numVias);
    cache->config.ciclosPenalidade = (int)json_get_int(valCache, "penaltyCycles", cache->config.ciclosPenalidade);
    cache->numConjuntos = (int)json_get_int(valCache, "numSets", cache->numConjuntos);

    if (cache->config.tamBloco > 0) {
        cache->totalLinhas = cache->config.capacidade / cache->config.tamBloco;
        if (cache->totalLinhas > MIC1_CACHE_MAX_LINES) {
            cache->totalLinhas = MIC1_CACHE_MAX_LINES;
        }
    }

    const JsonValue *st = json_object_get(valCache, "stats");
    if (st && st->type == JSON_OBJECT) {
        cache->stats.leiturasHit = (uint64_t)json_get_int(st, "readHits", 0);
        cache->stats.leiturasMiss = (uint64_t)json_get_int(st, "readMisses", 0);
        cache->stats.escritasHit = (uint64_t)json_get_int(st, "writeHits", 0);
        cache->stats.escritasMiss = (uint64_t)json_get_int(st, "writeMisses", 0);
        cache->stats.totalHits = (uint64_t)json_get_int(st, "totalHits", 0);
        cache->stats.totalMisses = (uint64_t)json_get_int(st, "totalMisses", 0);
        const JsonValue *taxaHitVal = json_object_get(st, "hitRate");
        cache->stats.taxaHit = (taxaHitVal && taxaHitVal->type == JSON_NUMBER) ? taxaHitVal->as.number : 0.0;
        const JsonValue *taxaMissVal = json_object_get(st, "missRate");
        cache->stats.taxaMiss = (taxaMissVal && taxaMissVal->type == JSON_NUMBER) ? taxaMissVal->as.number : 0.0;
        cache->stats.totalPenalidade = (uint64_t)json_get_int(st, "penaltyCyclesTotal", 0);
    }

    const JsonValue *ult = json_object_get(valCache, "lastAccess");
    if (ult && ult->type == JSON_OBJECT) {
        cache->ultimoAcesso.teveAcesso = json_get_bool(ult, "hasAccess", 0) != 0;
        snprintf(cache->ultimoAcesso.evento, sizeof(cache->ultimoAcesso.evento), "%s", json_get_string(ult, "event", ""));
        cache->ultimoAcesso.endereco = (uint16_t)json_get_int(ult, "address", 0);
        cache->ultimoAcesso.bloco = (uint16_t)json_get_int(ult, "block", 0);
        cache->ultimoAcesso.conjunto = (int)json_get_int(ult, "setIndex", 0);
        cache->ultimoAcesso.via = (int)json_get_int(ult, "wayIndex", 0);
        cache->ultimoAcesso.tag = (uint16_t)json_get_int(ult, "tag", 0);
        cache->ultimoAcesso.hit = json_get_bool(ult, "hit", 0) != 0;
    }

    const JsonValue *arrLinhas = json_get_array(valCache, "lines");
    if (arrLinhas && arrLinhas->type == JSON_ARRAY) {
        size_t len = json_array_length(arrLinhas);
        for (size_t i = 0; i < len && i < (size_t)cache->totalLinhas; i++) {
            const JsonValue *item = arrLinhas->as.array.items[i];
            if (!item || item->type != JSON_OBJECT) continue;
            int idx = (int)json_get_int(item, "index", (long long)i);
            if (idx >= 0 && idx < cache->totalLinhas) {
                Mic1CacheLine *l = &cache->linhas[idx];
                l->valido = json_get_bool(item, "valid", 0) != 0;
                l->sujo = json_get_bool(item, "dirty", 0) != 0;
                l->tag = (uint16_t)json_get_int(item, "tag", 0);
                l->endBloco = (uint16_t)json_get_int(item, "blockAddress", 0);
                l->ultimoAcesso = (uint64_t)json_get_int(item, "lastAccessTime", 0);
                l->tempoCarga = (uint64_t)json_get_int(item, "loadTime", 0);
                if (l->ultimoAcesso > cache->contAcessos) cache->contAcessos = l->ultimoAcesso;
                if (l->tempoCarga > cache->contAcessos) cache->contAcessos = l->tempoCarga;
                const JsonValue *arrPalavras = json_get_array(item, "words");
                if (arrPalavras && arrPalavras->type == JSON_ARRAY) {
                    size_t tamPalavras = json_array_length(arrPalavras);
                    for (size_t b = 0; b < tamPalavras && b < MIC1_CACHE_MAX_BLOCK_WORDS; b++) {
                        l->palavras[b] = (uint16_t)json_array_get_int(arrPalavras, b, 0);
                    }
                }
            }
        }
    }
}

static void state_json(JsonWriter *w,const Mic1State *c) {
    jw_obj_start(w);
    jw_key_int(w,"pc",c->r[R_PC]); jw_key_int(w,"ac",mic1_signed(c->r[R_AC])); jw_key_int(w,"sp",mic1_signed(c->r[R_SP]));
    jw_key_int(w,"ir",c->r[R_IR]); jw_key_int(w,"mar",c->mar); jw_key_int(w,"mbr",mic1_signed(c->mbr));
    jw_key_int(w,"mbrRead",mic1_signed(c->mbrLeitura)); jw_key_int(w,"mbrWrite",mic1_signed(c->mbrEscrita));
    jw_key(w,"registers"); jw_arr_start(w); for (int i=0;i<16;i++) jw_int(w,c->r[i]); jw_arr_end(w);
    jw_key(w,"memory"); jw_arr_start(w); for (int i=0;i<MIC1_MEM_SIZE;i++) jw_int(w,mic1_signed(c->memoria[i])); jw_arr_end(w);
    jw_key_int(w,"mir",c->mir); jw_key_int(w,"mpc",c->mpc);
    jw_key_int(w,"currentMicroAddress",c->endMicroAtual); jw_key_int(w,"nextSubcycle",c->proxSubciclo);
    jw_key_int(w,"latchA",c->latchA); jw_key_int(w,"latchB",c->latchB);
    jw_key_int(w,"aluA",c->ulaA); jw_key_int(w,"aluOut",c->saidaUla); jw_key_int(w,"shiftOut",c->saidaDeslocador);
    jw_key_int(w,"memoryPending",c->memPendente); jw_key_int(w,"pendingAddress",c->endPendente);
    jw_key_int(w,"pendingValue",c->valorPendente); jw_key_int(w,"readValue",c->valorLido);
    jw_key_int(w,"cyclesExecuted",c->ciclosExecutados); jw_key_int(w,"subcyclesExecuted",c->subciclosExecutados);
    jw_key_int(w,"instructionsExecuted",c->instrucoesExecutadas); jw_key_int(w,"instructionAddress",c->endInstrucao);
    jw_key_bool(w,"negative",c->negativo); jw_key_bool(w,"zero",c->zero);
    jw_key_bool(w,"readReady",c->leituraPronta); jw_key_bool(w,"halted",c->parado);
    jw_key_bool(w,"atInstructionBoundary",mic1_at_boundary(c));
    jw_key_str(w,"errorMessage",c->msgErro);
    jw_key_str(w,"microInstruction",MIC1_ROM_TEXT[c->endMicroAtual]);
    jw_key(w,"trace"); jw_arr_start(w);
    for (unsigned i=0;i<c->qtdHistorico;i++) {
        const MicroTraceEntry *t=&c->historico[i]; jw_obj_start(w); jw_key_str(w,"label",t->texto);
        jw_key_int(w,"mpc",t->mpc); jw_key_int(w,"subcycle",t->subciclo);
        jw_key_int(w,"pc",t->pc); jw_key_int(w,"ac",t->ac); jw_key_int(w,"sp",t->sp);
        jw_key_int(w,"mar",t->mar); jw_key_int(w,"mbr",t->mbr); jw_key_int(w,"ir",t->ir);
        jw_key_int(w,"writeAddress",t->endEscrita);
        jw_obj_end(w);
    }
    jw_arr_end(w);

    jw_key(w, "cache");
    cache_json(w, &c->cache);

    jw_obj_end(w);
}

static bool integer(const JsonValue *v,double min,double max) {
    return v && v->type==JSON_NUMBER && isfinite(v->as.number) && v->as.number>=min && v->as.number<=max && floor(v->as.number)==v->as.number;
}

static bool array_words(const JsonValue *arr,uint16_t *dest,size_t tam) {
    if (!arr || arr->type!=JSON_ARRAY || json_array_length(arr)!=tam) return false;
    for (size_t i=0;i<tam;i++) {
        const JsonValue *v=arr->as.array.items[i];
        if (!integer(v,-32768,65535)) return false;
        dest[i]=(uint16_t)(int)v->as.number;
    }
    return true;
}

static bool state_read(Mic1State *c,const JsonValue *s) {
    memset(c,0,sizeof(*c));
    if (!s || s->type!=JSON_OBJECT) return false;
    if (!array_words(json_get_array(s,"registers"),c->r,16) || !array_words(json_get_array(s,"memory"),c->memoria,MIC1_MEM_SIZE)) return false;
    if (c->r[R_ZERO]!=0 || c->r[R_ONE]!=1 || c->r[R_MINUS]!=65535 || c->r[R_AMASK]!=4095 || c->r[R_SMASK]!=255) return false;
#define READ_INT(chave,field,lo,hi) do { const JsonValue *v=json_object_get(s,chave); if (!integer(v,lo,hi)) return false; c->field=(uint32_t)(int64_t)v->as.number; } while(0)
    READ_INT("mar",mar,0,4095); READ_INT("mbr",mbr,-32768,65535);
    READ_INT("mir",mir,0,4294967295.0); READ_INT("mpc",mpc,0,MIC1_ROM_USED-1); READ_INT("currentMicroAddress",endMicroAtual,0,MIC1_ROM_USED-1); READ_INT("nextSubcycle",proxSubciclo,1,4);
    READ_INT("latchA",latchA,0,65535); READ_INT("latchB",latchB,0,65535); READ_INT("aluA",ulaA,0,65535); READ_INT("aluOut",saidaUla,0,65535); READ_INT("shiftOut",saidaDeslocador,0,65535);
    READ_INT("memoryPending",memPendente,0,2); READ_INT("pendingAddress",endPendente,0,4095); READ_INT("pendingValue",valorPendente,0,65535); READ_INT("readValue",valorLido,0,65535);
    READ_INT("cyclesExecuted",ciclosExecutados,0,1000000000); READ_INT("subcyclesExecuted",subciclosExecutados,0,4000000000.0); READ_INT("instructionsExecuted",instrucoesExecutadas,0,1000000000);
    READ_INT("instructionAddress",endInstrucao,0,4095);
#undef READ_INT
    /* MBR usa exibicao assinada. Evita conversao negativa double -> unsigned. */
    c->mbr=(uint16_t)(int)json_get_int(s,"mbr",0);
    const JsonValue *vmr=json_object_get(s,"mbrRead");
    c->mbrLeitura=vmr&&integer(vmr,-32768,65535)?(uint16_t)(int)vmr->as.number:c->mbr;
    const JsonValue *vmw=json_object_get(s,"mbrWrite");
    c->mbrEscrita=vmw&&integer(vmw,-32768,65535)?(uint16_t)(int)vmw->as.number:c->mbr;
#define READ_BOOL(chave,field) do { const JsonValue *v=json_object_get(s,chave); if (!v || v->type!=JSON_BOOL) return false; c->field=v->as.boolean!=0; } while(0)
    READ_BOOL("negative",negativo); READ_BOOL("zero",zero); READ_BOOL("readReady",leituraPronta); READ_BOOL("halted",parado);
#undef READ_BOOL
    snprintf(c->msgErro,sizeof(c->msgErro),"%s",json_get_string(s,"errorMessage",""));
    /* Controle vem da ROM fixa; nao aceitar uma MIR adulterada via HTTP. */
    if (c->mir!=MIC1_ROM[c->endMicroAtual] && !(c->ciclosExecutados==0 && c->subciclosExecutados==0 && c->mir==0)) return false;

    const JsonValue *valCache = json_object_get(s, "cache");
    cache_init_default(&c->cache);
    if (valCache) {
        cache_from_json(&c->cache, valCache);
        if (c->cache.totalLinhas > MIC1_CACHE_MAX_LINES) {
            c->cache.totalLinhas = MIC1_CACHE_MAX_LINES;
        }
    } else {
        if (configCacheIniciada) {
            cache_configure(&c->cache, &configCachePadrao, c->memoria);
        }
    }

    return true;
}

char *protocol_microcode(void) {
    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key(&w,"microcode"); jw_arr_start(&w);
    for (int i=0;i<MIC1_ROM_USED;i++) {
        jw_obj_start(&w); jw_key_int(&w,"address",i); jw_key_int(&w,"word",MIC1_ROM[i]); jw_key_str(&w,"text",MIC1_ROM_TEXT[i]); jw_obj_end(&w);
    }
    jw_arr_end(&w); jw_obj_end(&w); char *saida=jw_take(&w); jw_free(&w); return saida;
}

char *protocol_request(const char *rota,const char *corpo,int *status) {
    char erro[200]; *status=400;
    JsonValue *req=json_parse(corpo,erro,sizeof(erro));
    if (!req || req->type!=JSON_OBJECT) { if (req) json_free(req); return protocol_error("Pedido JSON invalido."); }
    char *saida=NULL;
    if (!strcmp(rota,"/api/assemble")) {
        const JsonValue *codigo=json_object_get(req,"sourceCode");
        if (!codigo || codigo->type!=JSON_STRING || strlen(codigo->as.string)>512*1024) saida=protocol_error("sourceCode deve ser texto de ate 512 KiB.");
        else {
            saida=assemble_json(codigo->as.string,NULL);
            *status=200;
        }
    } else {
        Mic1State *c=calloc(1,sizeof(*c));
        if (!c) { json_free(req); *status=500; return protocol_error("Sem memoria."); }
        bool valido=false;

        if (!strcmp(rota,"/api/sim/init")) {
            const JsonValue *p=json_get_array(req,"program"); size_t tam=json_array_length(p);
            uint16_t palavras[MIC1_MEM_SIZE];
            if (tam && tam<=MIC1_MEM_SIZE && array_words(p,palavras,tam)) {
                int16_t palavrasSinal[MIC1_MEM_SIZE];
                for (size_t i=0;i<tam;i++) palavrasSinal[i]=(int16_t)mic1_signed(palavras[i]);
                mic1_init(c,palavrasSinal,tam);
                if (configCacheIniciada) {
                    cache_configure(&c->cache, &configCachePadrao, c->memoria);
                }
                const JsonValue *valCache = json_object_get(req, "cache");
                if (valCache) {
                    cache_from_json(&c->cache, valCache);
                }
                valido=true;
            }
        } else if (!strcmp(rota,"/api/sim/step-macro") || !strcmp(rota,"/api/sim/step-micro") || !strcmp(rota,"/api/sim/step-subcycle")) {
            valido=state_read(c,json_object_get(req,"state"));
            const JsonValue *valCache = json_object_get(req, "cache");
            if (valCache) cache_from_json(&c->cache, valCache);
            if (valido) {
                if (!strcmp(rota,"/api/sim/step-macro")) mic1_step_macro(c);
                else if (!strcmp(rota,"/api/sim/step-micro")) mic1_step_micro(c);
                else mic1_step_subcycle(c);
            }
        } else if (!strcmp(rota, "/api/sim/run")) {
            valido = state_read(c, json_object_get(req, "state"));
            const JsonValue *valCache = json_object_get(req, "cache");
            if (valCache) cache_from_json(&c->cache, valCache);
            if (valido) {
                int maxCiclos = (int)json_get_int(req, "maxCycles", 10000);
                if (maxCiclos <= 0 || maxCiclos > 1000000) maxCiclos = 10000;
                for (int i = 0; i < maxCiclos && !c->parado; i++) {
                    mic1_step_macro(c);
                }
            }
        } else if (!strcmp(rota, "/api/sim/cache-config")) {
            const JsonValue *st = json_object_get(req, "state");
            if (st) {
                valido = state_read(c, st);
                const JsonValue *valCache = json_object_get(req, "cache");
                if (valCache) cache_from_json(&c->cache, valCache);
            } else {
                mic1_init(c, NULL, 0);
                if (configCacheIniciada) {
                    cache_configure(&c->cache, &configCachePadrao, c->memoria);
                }
                valido = true;
            }
            if (valido) {
                Mic1CacheConfig cfg = c->cache.config;
                const JsonValue *objCfg = json_object_get(req, "config");
                if (!objCfg || objCfg->type != JSON_OBJECT) objCfg = req;

                cfg.ativa = json_get_bool(objCfg, "enabled", cfg.ativa) != 0;
                const char *m = json_get_string(objCfg, "mapping", "");
                if (!strcmp(m, "direct")) cfg.mapeamento = CACHE_MAP_DIRECT;
                else if (!strcmp(m, "fully")) cfg.mapeamento = CACHE_MAP_FULLY_ASSOCIATIVE;
                else if (!strcmp(m, "set")) cfg.mapeamento = CACHE_MAP_SET_ASSOCIATIVE;

                const char *r = json_get_string(objCfg, "replacement", "");
                if (!strcmp(r, "lru")) cfg.substituicao = CACHE_REPLACE_LRU;
                else if (!strcmp(r, "fifo")) cfg.substituicao = CACHE_REPLACE_FIFO;

                const char *p = json_get_string(objCfg, "writePolicy", "");
                if (!strcmp(p, "write-through")) cfg.politicaEscrita = CACHE_WRITE_THROUGH;
                else if (!strcmp(p, "write-back")) cfg.politicaEscrita = CACHE_WRITE_BACK;

                cfg.capacidade = (int)json_get_int(objCfg, "capacityWords", cfg.capacidade);
                cfg.tamBloco = (int)json_get_int(objCfg, "blockSizeWords", cfg.tamBloco);
                cfg.numVias = (int)json_get_int(objCfg, "numWays", cfg.numVias);
                cfg.ciclosPenalidade = (int)json_get_int(objCfg, "penaltyCycles", cfg.ciclosPenalidade);

                if (!cache_configure(&c->cache, &cfg, c->memoria)) {
                    valido = false;
                    saida = protocol_error("Configuracao de cache invalida.");
                    *status = 400;
                } else {
                    configCachePadrao = c->cache.config;
                    configCacheIniciada = true;
                }
            }
        }

        if (!valido) {
            if (!saida) saida=protocol_error("Programa, estado da CPU ou rota invalido. Reinicie a montagem.");
        } else {
            JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w,"ok",true);
            jw_key(&w,"state"); state_json(&w,c);
            jw_key(&w,"cache"); cache_json(&w,&c->cache);
            jw_obj_end(&w); saida=jw_take(&w); jw_free(&w); *status=200;
        }
        free(c);
    }
    json_free(req); if (!saida) { *status=500; saida=protocol_error("Sem memoria."); } return saida;
}
