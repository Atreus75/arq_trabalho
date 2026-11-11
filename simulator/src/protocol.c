#include "protocol.h"
#include "mic1.h"
#include "assembler.h"
#include "json.h"
#include "cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

static Mic1CacheConfig s_defaultCacheConfig;
static bool s_defaultCacheConfigInit = false;

char *protocol_error(const char *msg) {
    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w,"ok",false);
    jw_key_str(&w,"errorMessage",msg); jw_obj_end(&w); char *s=jw_take(&w); jw_free(&w); return s;
}

static void jw_key_double(JsonWriter *w, const char *key, double val) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.4f", val);
    char *comma = strchr(tmp, ',');
    if (comma) *comma = '.';
    jw_key(w, key);
    size_t n = strlen(tmp);
    if (w->len + n + 1 > w->cap) {
        size_t newCap = (w->cap + n + 1) * 2;
        char *nb = realloc(w->buf, newCap);
        if (nb) { w->buf = nb; w->cap = newCap; }
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
    jw_key_bool(w, "enabled", cache->config.enabled);

    const char *mappingStr = "direct";
    if (cache->config.mapping == CACHE_MAP_FULLY_ASSOCIATIVE) mappingStr = "fully";
    else if (cache->config.mapping == CACHE_MAP_SET_ASSOCIATIVE) mappingStr = "set";
    jw_key_str(w, "mapping", mappingStr);

    const char *replaceStr = (cache->config.replacement == CACHE_REPLACE_FIFO) ? "fifo" : "lru";
    jw_key_str(w, "replacement", replaceStr);

    const char *policyStr = (cache->config.writePolicy == CACHE_WRITE_BACK) ? "write-back" : "write-through";
    jw_key_str(w, "writePolicy", policyStr);

    jw_key_int(w, "capacityWords", cache->config.capacityWords);
    jw_key_int(w, "blockSizeWords", cache->config.blockSizeWords);
    jw_key_int(w, "numSets", cache->numSets);
    jw_key_int(w, "numWays", cache->config.numWays);
    jw_key_int(w, "penaltyCycles", cache->config.penaltyCycles);

    /* stats */
    jw_key(w, "stats");
    jw_obj_start(w);
    jw_key_int(w, "readHits", (long long)cache->stats.readHits);
    jw_key_int(w, "readMisses", (long long)cache->stats.readMisses);
    jw_key_int(w, "writeHits", (long long)cache->stats.writeHits);
    jw_key_int(w, "writeMisses", (long long)cache->stats.writeMisses);
    jw_key_int(w, "totalHits", (long long)cache->stats.totalHits);
    jw_key_int(w, "totalMisses", (long long)cache->stats.totalMisses);
    jw_key_double(w, "hitRate", cache->stats.hitRate);
    jw_key_double(w, "missRate", cache->stats.missRate);
    jw_key_int(w, "penaltyCyclesTotal", (long long)cache->stats.penaltyCyclesTotal);
    jw_obj_end(w);

    /* lastAccess */
    jw_key(w, "lastAccess");
    jw_obj_start(w);
    jw_key_bool(w, "hasAccess", cache->lastAccess.hasAccess);
    jw_key_str(w, "event", cache->lastAccess.event);
    jw_key_int(w, "address", cache->lastAccess.address);
    jw_key_int(w, "block", cache->lastAccess.block);
    jw_key_int(w, "setIndex", cache->lastAccess.setIndex);
    jw_key_int(w, "wayIndex", cache->lastAccess.wayIndex);
    jw_key_int(w, "tag", cache->lastAccess.tag);
    jw_key_bool(w, "hit", cache->lastAccess.hit);
    jw_obj_end(w);

    /* lines */
    jw_key(w, "lines");
    jw_arr_start(w);
    int total = cache->numLinesTotal;
    int ways = cache->config.numWays > 0 ? cache->config.numWays : 1;
    for (int i = 0; i < total; i++) {
        const Mic1CacheLine *l = &cache->lines[i];
        jw_obj_start(w);
        jw_key_int(w, "index", i);
        jw_key_int(w, "set", i / ways);
        jw_key_int(w, "way", i % ways);
        jw_key_bool(w, "valid", l->valid);
        jw_key_bool(w, "dirty", l->dirty);
        jw_key_int(w, "tag", l->tag);
        jw_key_int(w, "blockAddress", l->blockAddress);

        jw_key(w, "words");
        jw_arr_start(w);
        int blockSize = cache->config.blockSizeWords;
        for (int b = 0; b < blockSize; b++) {
            jw_int(w, l->words[b]);
        }
        jw_arr_end(w);

        jw_key_int(w, "lastAccessTime", (long long)l->lastAccessTime);
        jw_key_int(w, "loadTime", (long long)l->loadTime);
        jw_obj_end(w);
    }
    jw_arr_end(w);

    jw_obj_end(w);
}

static void cache_from_json(Mic1Cache *cache, const JsonValue *cval) {
    if (!cval || cval->type != JSON_OBJECT) return;

    cache->config.enabled = json_get_bool(cval, "enabled", cache->config.enabled) != 0;

    const char *m = json_get_string(cval, "mapping", "");
    if (!strcmp(m, "direct")) cache->config.mapping = CACHE_MAP_DIRECT;
    else if (!strcmp(m, "fully")) cache->config.mapping = CACHE_MAP_FULLY_ASSOCIATIVE;
    else if (!strcmp(m, "set")) cache->config.mapping = CACHE_MAP_SET_ASSOCIATIVE;

    const char *r = json_get_string(cval, "replacement", "");
    if (!strcmp(r, "lru")) cache->config.replacement = CACHE_REPLACE_LRU;
    else if (!strcmp(r, "fifo")) cache->config.replacement = CACHE_REPLACE_FIFO;

    const char *p = json_get_string(cval, "writePolicy", "");
    if (!strcmp(p, "write-through")) cache->config.writePolicy = CACHE_WRITE_THROUGH;
    else if (!strcmp(p, "write-back")) cache->config.writePolicy = CACHE_WRITE_BACK;

    cache->config.capacityWords = (int)json_get_int(cval, "capacityWords", cache->config.capacityWords);
    cache->config.blockSizeWords = (int)json_get_int(cval, "blockSizeWords", cache->config.blockSizeWords);
    cache->config.numWays = (int)json_get_int(cval, "numWays", cache->config.numWays);
    cache->config.penaltyCycles = (int)json_get_int(cval, "penaltyCycles", cache->config.penaltyCycles);
    cache->numSets = (int)json_get_int(cval, "numSets", cache->numSets);

    if (cache->config.blockSizeWords > 0) {
        cache->numLinesTotal = cache->config.capacityWords / cache->config.blockSizeWords;
        if (cache->numLinesTotal > MIC1_CACHE_MAX_LINES) {
            cache->numLinesTotal = MIC1_CACHE_MAX_LINES;
        }
    }

    const JsonValue *st = json_object_get(cval, "stats");
    if (st && st->type == JSON_OBJECT) {
        cache->stats.readHits = (uint64_t)json_get_int(st, "readHits", 0);
        cache->stats.readMisses = (uint64_t)json_get_int(st, "readMisses", 0);
        cache->stats.writeHits = (uint64_t)json_get_int(st, "writeHits", 0);
        cache->stats.writeMisses = (uint64_t)json_get_int(st, "writeMisses", 0);
        cache->stats.totalHits = (uint64_t)json_get_int(st, "totalHits", 0);
        cache->stats.totalMisses = (uint64_t)json_get_int(st, "totalMisses", 0);
        const JsonValue *hr = json_object_get(st, "hitRate");
        cache->stats.hitRate = (hr && hr->type == JSON_NUMBER) ? hr->as.number : 0.0;
        const JsonValue *mr = json_object_get(st, "missRate");
        cache->stats.missRate = (mr && mr->type == JSON_NUMBER) ? mr->as.number : 0.0;
        cache->stats.penaltyCyclesTotal = (uint64_t)json_get_int(st, "penaltyCyclesTotal", 0);
    }

    const JsonValue *la = json_object_get(cval, "lastAccess");
    if (la && la->type == JSON_OBJECT) {
        cache->lastAccess.hasAccess = json_get_bool(la, "hasAccess", 0) != 0;
        snprintf(cache->lastAccess.event, sizeof(cache->lastAccess.event), "%s", json_get_string(la, "event", ""));
        cache->lastAccess.address = (uint16_t)json_get_int(la, "address", 0);
        cache->lastAccess.block = (uint16_t)json_get_int(la, "block", 0);
        cache->lastAccess.setIndex = (int)json_get_int(la, "setIndex", 0);
        cache->lastAccess.wayIndex = (int)json_get_int(la, "wayIndex", 0);
        cache->lastAccess.tag = (uint16_t)json_get_int(la, "tag", 0);
        cache->lastAccess.hit = json_get_bool(la, "hit", 0) != 0;
    }

    const JsonValue *linesArr = json_get_array(cval, "lines");
    if (linesArr && linesArr->type == JSON_ARRAY) {
        size_t len = json_array_length(linesArr);
        for (size_t i = 0; i < len && i < (size_t)cache->numLinesTotal; i++) {
            const JsonValue *item = linesArr->as.array.items[i];
            if (!item || item->type != JSON_OBJECT) continue;
            int idx = (int)json_get_int(item, "index", (long long)i);
            if (idx >= 0 && idx < cache->numLinesTotal) {
                Mic1CacheLine *l = &cache->lines[idx];
                l->valid = json_get_bool(item, "valid", 0) != 0;
                l->dirty = json_get_bool(item, "dirty", 0) != 0;
                l->tag = (uint16_t)json_get_int(item, "tag", 0);
                l->blockAddress = (uint16_t)json_get_int(item, "blockAddress", 0);
                l->lastAccessTime = (uint64_t)json_get_int(item, "lastAccessTime", 0);
                l->loadTime = (uint64_t)json_get_int(item, "loadTime", 0);
                if (l->lastAccessTime > cache->accessCounter) cache->accessCounter = l->lastAccessTime;
                if (l->loadTime > cache->accessCounter) cache->accessCounter = l->loadTime;
                const JsonValue *wArr = json_get_array(item, "words");
                if (wArr && wArr->type == JSON_ARRAY) {
                    size_t wlen = json_array_length(wArr);
                    for (size_t b = 0; b < wlen && b < MIC1_CACHE_MAX_BLOCK_WORDS; b++) {
                        l->words[b] = (uint16_t)json_array_get_int(wArr, b, 0);
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
    jw_key_int(w,"mbrRead",mic1_signed(c->mbrRead)); jw_key_int(w,"mbrWrite",mic1_signed(c->mbrWrite));
    jw_key(w,"registers"); jw_arr_start(w); for (int i=0;i<16;i++) jw_int(w,c->r[i]); jw_arr_end(w);
    jw_key(w,"memory"); jw_arr_start(w); for (int i=0;i<MIC1_MEM_SIZE;i++) jw_int(w,mic1_signed(c->memory[i])); jw_arr_end(w);
#define OUT_INT(n) jw_key_int(w,#n,c->n)
    OUT_INT(mir); OUT_INT(mpc); OUT_INT(currentMicroAddress); OUT_INT(nextSubcycle);
    OUT_INT(latchA); OUT_INT(latchB); OUT_INT(aluA); OUT_INT(aluOut); OUT_INT(shiftOut);
    OUT_INT(memoryPending); OUT_INT(pendingAddress); OUT_INT(pendingValue); OUT_INT(readValue);
    OUT_INT(cyclesExecuted); OUT_INT(subcyclesExecuted); OUT_INT(instructionsExecuted); OUT_INT(instructionAddress);
#undef OUT_INT
    jw_key_bool(w,"negative",c->negative); jw_key_bool(w,"zero",c->zero);
    jw_key_bool(w,"readReady",c->readReady); jw_key_bool(w,"halted",c->halted);
    jw_key_bool(w,"atInstructionBoundary",mic1_at_boundary(c));
    jw_key_str(w,"errorMessage",c->errorMessage);
    jw_key_str(w,"microInstruction",MIC1_ROM_TEXT[c->currentMicroAddress]);
    jw_key(w,"trace"); jw_arr_start(w);
    for (unsigned i=0;i<c->traceCount;i++) {
        const MicroTraceEntry *t=&c->trace[i]; jw_obj_start(w); jw_key_str(w,"label",t->label);
#define OUT_TRACE(n) jw_key_int(w,#n,t->n)
        OUT_TRACE(mpc); OUT_TRACE(subcycle); OUT_TRACE(pc); OUT_TRACE(ac); OUT_TRACE(sp);
        OUT_TRACE(mar); OUT_TRACE(mbr); OUT_TRACE(ir); OUT_TRACE(writeAddress);
#undef OUT_TRACE
        jw_obj_end(w);
    }
    jw_arr_end(w);

    jw_key(w, "cache");
    cache_json(w, &c->cache);

    jw_obj_end(w);
}

static bool integer(const JsonValue *v,double low,double high) {
    return v && v->type==JSON_NUMBER && isfinite(v->as.number) && v->as.number>=low && v->as.number<=high && floor(v->as.number)==v->as.number;
}

static bool array_words(const JsonValue *array,uint16_t *dest,size_t size) {
    if (!array || array->type!=JSON_ARRAY || json_array_length(array)!=size) return false;
    for (size_t i=0;i<size;i++) {
        const JsonValue *v=array->as.array.items[i];
        if (!integer(v,-32768,65535)) return false;
        dest[i]=(uint16_t)(int)v->as.number;
    }
    return true;
}

static bool state_read(Mic1State *c,const JsonValue *s) {
    memset(c,0,sizeof(*c));
    if (!s || s->type!=JSON_OBJECT) return false;
    if (!array_words(json_get_array(s,"registers"),c->r,16) || !array_words(json_get_array(s,"memory"),c->memory,MIC1_MEM_SIZE)) return false;
    if (c->r[R_ZERO]!=0 || c->r[R_ONE]!=1 || c->r[R_MINUS]!=65535 || c->r[R_AMASK]!=4095 || c->r[R_SMASK]!=255) return false;
#define READ_INT(n,lo,hi) do { const JsonValue *v=json_object_get(s,#n); if (!integer(v,lo,hi)) return false; c->n=(uint32_t)(int64_t)v->as.number; } while(0)
    READ_INT(mar,0,4095); READ_INT(mbr,-32768,65535);
    READ_INT(mir,0,4294967295.0); READ_INT(mpc,0,MIC1_ROM_USED-1); READ_INT(currentMicroAddress,0,MIC1_ROM_USED-1); READ_INT(nextSubcycle,1,4);
    READ_INT(latchA,0,65535); READ_INT(latchB,0,65535); READ_INT(aluA,0,65535); READ_INT(aluOut,0,65535); READ_INT(shiftOut,0,65535);
    READ_INT(memoryPending,0,2); READ_INT(pendingAddress,0,4095); READ_INT(pendingValue,0,65535); READ_INT(readValue,0,65535);
    READ_INT(cyclesExecuted,0,1000000000); READ_INT(subcyclesExecuted,0,4000000000.0); READ_INT(instructionsExecuted,0,1000000000);
    READ_INT(instructionAddress,0,4095);
#undef READ_INT
    /* MBR usa exibicao assinada. Evita conversao negativa double -> unsigned. */
    c->mbr=(uint16_t)(int)json_get_int(s,"mbr",0);
    const JsonValue *vmr=json_object_get(s,"mbrRead");
    c->mbrRead=vmr&&integer(vmr,-32768,65535)?(uint16_t)(int)vmr->as.number:c->mbr;
    const JsonValue *vmw=json_object_get(s,"mbrWrite");
    c->mbrWrite=vmw&&integer(vmw,-32768,65535)?(uint16_t)(int)vmw->as.number:c->mbr;
#define READ_BOOL(n) do { const JsonValue *v=json_object_get(s,#n); if (!v || v->type!=JSON_BOOL) return false; c->n=v->as.boolean!=0; } while(0)
    READ_BOOL(negative); READ_BOOL(zero); READ_BOOL(readReady); READ_BOOL(halted);
#undef READ_BOOL
    snprintf(c->errorMessage,sizeof(c->errorMessage),"%s",json_get_string(s,"errorMessage",""));
    /* Controle vem da ROM fixa; nao aceitar uma MIR adulterada via HTTP. */
    if (c->mir!=MIC1_ROM[c->currentMicroAddress] && !(c->cyclesExecuted==0 && c->subcyclesExecuted==0 && c->mir==0)) return false;

    const JsonValue *cval = json_object_get(s, "cache");
    cache_init_default(&c->cache);
    if (cval) {
        cache_from_json(&c->cache, cval);
        if (c->cache.numLinesTotal > MIC1_CACHE_MAX_LINES) {
            c->cache.numLinesTotal = MIC1_CACHE_MAX_LINES;
        }
    } else {
        if (s_defaultCacheConfigInit) {
            cache_configure(&c->cache, &s_defaultCacheConfig, c->memory);
        }
    }

    return true;
}

char *protocol_microcode(void) {
    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key(&w,"microcode"); jw_arr_start(&w);
    for (int i=0;i<MIC1_ROM_USED;i++) {
        jw_obj_start(&w); jw_key_int(&w,"address",i); jw_key_int(&w,"word",MIC1_ROM[i]); jw_key_str(&w,"text",MIC1_ROM_TEXT[i]); jw_obj_end(&w);
    }
    jw_arr_end(&w); jw_obj_end(&w); char *out=jw_take(&w); jw_free(&w); return out;
}

char *protocol_request(const char *route,const char *body,int *status) {
    char error[200]; *status=400;
    JsonValue *req=json_parse(body,error,sizeof(error));
    if (!req || req->type!=JSON_OBJECT) { if (req) json_free(req); return protocol_error("Pedido JSON invalido."); }
    char *out=NULL;
    if (!strcmp(route,"/api/assemble")) {
        const JsonValue *src=json_object_get(req,"sourceCode");
        if (!src || src->type!=JSON_STRING || strlen(src->as.string)>512*1024) out=protocol_error("sourceCode deve ser texto de ate 512 KiB.");
        else {
            out=assemble_json(src->as.string,NULL);
            *status=200;
        }
    } else {
        Mic1State *c=calloc(1,sizeof(*c));
        if (!c) { json_free(req); *status=500; return protocol_error("Sem memoria."); }
        bool valid=false;

        if (!strcmp(route,"/api/sim/init")) {
            const JsonValue *p=json_get_array(req,"program"); size_t size=json_array_length(p);
            uint16_t words[MIC1_MEM_SIZE];
            if (size && size<=MIC1_MEM_SIZE && array_words(p,words,size)) {
                int16_t signedWords[MIC1_MEM_SIZE];
                for (size_t i=0;i<size;i++) signedWords[i]=(int16_t)mic1_signed(words[i]);
                mic1_init(c,signedWords,size);
                if (s_defaultCacheConfigInit) {
                    cache_configure(&c->cache, &s_defaultCacheConfig, c->memory);
                }
                const JsonValue *cval = json_object_get(req, "cache");
                if (cval) {
                    cache_from_json(&c->cache, cval);
                }
                valid=true;
            }
        } else if (!strcmp(route,"/api/sim/step-macro") || !strcmp(route,"/api/sim/step-micro") || !strcmp(route,"/api/sim/step-subcycle")) {
            valid=state_read(c,json_object_get(req,"state"));
            const JsonValue *cval = json_object_get(req, "cache");
            if (cval) cache_from_json(&c->cache, cval);
            if (valid) {
                if (!strcmp(route,"/api/sim/step-macro")) mic1_step_macro(c);
                else if (!strcmp(route,"/api/sim/step-micro")) mic1_step_micro(c);
                else mic1_step_subcycle(c);
            }
        } else if (!strcmp(route, "/api/sim/run")) {
            valid = state_read(c, json_object_get(req, "state"));
            const JsonValue *cval = json_object_get(req, "cache");
            if (cval) cache_from_json(&c->cache, cval);
            if (valid) {
                int maxCycles = (int)json_get_int(req, "maxCycles", 10000);
                if (maxCycles <= 0 || maxCycles > 1000000) maxCycles = 10000;
                for (int i = 0; i < maxCycles && !c->halted; i++) {
                    mic1_step_macro(c);
                }
            }
        } else if (!strcmp(route, "/api/sim/cache-config")) {
            const JsonValue *st = json_object_get(req, "state");
            if (st) {
                valid = state_read(c, st);
                const JsonValue *cval = json_object_get(req, "cache");
                if (cval) cache_from_json(&c->cache, cval);
            } else {
                mic1_init(c, NULL, 0);
                if (s_defaultCacheConfigInit) {
                    cache_configure(&c->cache, &s_defaultCacheConfig, c->memory);
                }
                valid = true;
            }
            if (valid) {
                Mic1CacheConfig cfg = c->cache.config;
                const JsonValue *cfgObj = json_object_get(req, "config");
                if (!cfgObj || cfgObj->type != JSON_OBJECT) cfgObj = req;

                cfg.enabled = json_get_bool(cfgObj, "enabled", cfg.enabled) != 0;
                const char *m = json_get_string(cfgObj, "mapping", "");
                if (!strcmp(m, "direct")) cfg.mapping = CACHE_MAP_DIRECT;
                else if (!strcmp(m, "fully")) cfg.mapping = CACHE_MAP_FULLY_ASSOCIATIVE;
                else if (!strcmp(m, "set")) cfg.mapping = CACHE_MAP_SET_ASSOCIATIVE;

                const char *r = json_get_string(cfgObj, "replacement", "");
                if (!strcmp(r, "lru")) cfg.replacement = CACHE_REPLACE_LRU;
                else if (!strcmp(r, "fifo")) cfg.replacement = CACHE_REPLACE_FIFO;

                const char *p = json_get_string(cfgObj, "writePolicy", "");
                if (!strcmp(p, "write-through")) cfg.writePolicy = CACHE_WRITE_THROUGH;
                else if (!strcmp(p, "write-back")) cfg.writePolicy = CACHE_WRITE_BACK;

                cfg.capacityWords = (int)json_get_int(cfgObj, "capacityWords", cfg.capacityWords);
                cfg.blockSizeWords = (int)json_get_int(cfgObj, "blockSizeWords", cfg.blockSizeWords);
                cfg.numWays = (int)json_get_int(cfgObj, "numWays", cfg.numWays);
                cfg.penaltyCycles = (int)json_get_int(cfgObj, "penaltyCycles", cfg.penaltyCycles);

                if (!cache_configure(&c->cache, &cfg, c->memory)) {
                    valid = false;
                    out = protocol_error("Configuracao de cache invalida.");
                    *status = 400;
                } else {
                    s_defaultCacheConfig = c->cache.config;
                    s_defaultCacheConfigInit = true;
                }
            }
        }

        if (!valid) {
            if (!out) out=protocol_error("Programa, estado da CPU ou rota invalido. Reinicie a montagem.");
        } else {
            JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w,"ok",true);
            jw_key(&w,"state"); state_json(&w,c);
            jw_key(&w,"cache"); cache_json(&w,&c->cache);
            jw_obj_end(&w); out=jw_take(&w); jw_free(&w); *status=200;
        }
        free(c);
    }
    json_free(req); if (!out) { *status=500; out=protocol_error("Sem memoria."); } return out;
}
