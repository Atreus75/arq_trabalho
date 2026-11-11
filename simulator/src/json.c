#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ============================= PARSER ============================= */

typedef struct {
    const char *s;
    size_t pos;
    size_t len;
    char *errbuf;
    size_t errbuf_size;
    int failed;
    int depth;
} Parser;

static void p_set_error(Parser *p, const char *msg) {
    if (p->failed) return; /* mantém o primeiro erro */
    p->failed = 1;
    if (p->errbuf && p->errbuf_size > 0) {
        snprintf(p->errbuf, p->errbuf_size, "erro de parse JSON na posicao %zu: %s",
                 p->pos, msg);
    }
}

static void p_skip_ws(Parser *p) {
    while (p->pos < p->len) {
        char c = p->s[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            p->pos++;
        } else {
            break;
        }
    }
}

static int p_peek(Parser *p) {
    if (p->pos >= p->len) return -1;
    return (unsigned char)p->s[p->pos];
}

static JsonValue *json_alloc(JsonType type) {
    JsonValue *v = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (!v) return NULL;
    v->type = type;
    return v;
}

static JsonValue *p_parse_value(Parser *p);

static JsonValue *p_parse_literal(Parser *p, const char *lit, JsonValue *result) {
    size_t n = strlen(lit);
    if (p->pos + n <= p->len && strncmp(p->s + p->pos, lit, n) == 0) {
        p->pos += n;
        return result;
    }
    p_set_error(p, "literal invalido");
    if (result) json_free(result);
    return NULL;
}

static JsonValue *p_parse_string_raw(Parser *p, char **out) {
    if (p_peek(p) != '"') {
        p_set_error(p, "string esperada");
        return NULL;
    }
    p->pos++; /* consome '"' */
    size_t cap = 32, len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { p_set_error(p, "sem memoria"); return NULL; }

    while (p->pos < p->len && p->s[p->pos] != '"') {
        char c = p->s[p->pos];
        if (c == '\\') {
            p->pos++;
            if (p->pos >= p->len) { p_set_error(p, "escape incompleto"); free(buf); return NULL; }
            char esc = p->s[p->pos];
            char decoded = 0;
            switch (esc) {
                case '"': decoded = '"'; break;
                case '\\': decoded = '\\'; break;
                case '/': decoded = '/'; break;
                case 'n': decoded = '\n'; break;
                case 't': decoded = '\t'; break;
                case 'r': decoded = '\r'; break;
                case 'b': decoded = '\b'; break;
                case 'f': decoded = '\f'; break;
                case 'u': {
                    unsigned code=0;
                    for (int j=0;j<4;j++) {
                        if (++p->pos>=p->len || !isxdigit((unsigned char)p->s[p->pos])) { p_set_error(p,"escape Unicode invalido"); free(buf);return NULL; }
                        int h=tolower((unsigned char)p->s[p->pos]);code=(code<<4)+(h<='9'?h-'0':h-'a'+10);
                    }
                    if (code>=0xd800 && code<=0xdbff) {
                        if (p->pos+2>=p->len || p->s[p->pos+1]!='\\' || p->s[p->pos+2]!='u') { p_set_error(p,"par Unicode incompleto");free(buf);return NULL; }
                        p->pos+=2;unsigned low=0;
                        for (int j=0;j<4;j++) {
                            if (++p->pos>=p->len || !isxdigit((unsigned char)p->s[p->pos])) { p_set_error(p,"Unicode invalido");free(buf);return NULL; }
                            int h=tolower((unsigned char)p->s[p->pos]);low=(low<<4)+(h<='9'?h-'0':h-'a'+10);
                        }
                        if (low<0xdc00||low>0xdfff) { p_set_error(p,"par Unicode invalido");free(buf);return NULL; }
                        code=0x10000+((code-0xd800)<<10)+(low-0xdc00);
                    } else if (code>=0xdc00&&code<=0xdfff) { p_set_error(p,"Unicode invalido");free(buf);return NULL; }
                    if (!code) { p_set_error(p,"NUL nao permitido em texto");free(buf);return NULL; }
                    if (len+5>=cap) {cap*=2;char *next=realloc(buf,cap);if(!next){free(buf);return NULL;}buf=next;}
                    if (code<128) decoded=(char)code;
                    else if (code<2048) {buf[len++]=(char)(0xc0|(code>>6));decoded=(char)(0x80|(code&63));}
                    else if (code<65536) {buf[len++]=(char)(0xe0|(code>>12));buf[len++]=(char)(0x80|((code>>6)&63));decoded=(char)(0x80|(code&63));}
                    else {buf[len++]=(char)(0xf0|(code>>18));buf[len++]=(char)(0x80|((code>>12)&63));buf[len++]=(char)(0x80|((code>>6)&63));decoded=(char)(0x80|(code&63));}
                    break;
                }
                default:
                    p_set_error(p,"escape invalido");free(buf);return NULL;
            }
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = decoded;
            p->pos++;
        } else {
            if ((unsigned char)c < 32) {p_set_error(p,"controle invalido em string");free(buf);return NULL;}
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = c;
            p->pos++;
        }
    }
    if (p->pos >= p->len) { p_set_error(p, "string nao terminada"); free(buf); return NULL; }
    p->pos++; /* consome '"' final */
    buf[len] = '\0';
    *out = buf;
    return (JsonValue *)1; /* sentinela de sucesso, valor real fica em *out */
}

static JsonValue *p_parse_string(Parser *p) {
    char *s = NULL;
    if (!p_parse_string_raw(p, &s)) return NULL;
    JsonValue *v = json_alloc(JSON_STRING);
    if (!v) { free(s); return NULL; }
    v->as.string = s;
    return v;
}

static JsonValue *p_parse_number(Parser *p) {
    size_t start=p->pos;
    if (p_peek(p)=='-')p->pos++;
    if (p_peek(p)=='0')p->pos++;
    else {
        if (p_peek(p)<'1'||p_peek(p)>'9')goto bad;
        while (p->pos<p->len && isdigit((unsigned char)p->s[p->pos]))p->pos++;
    }
    if (p_peek(p)=='.') {
        p->pos++;if (p_peek(p)<'0'||p_peek(p)>'9')goto bad;
        while(p->pos<p->len && isdigit((unsigned char)p->s[p->pos]))p->pos++;
    }
    if (p_peek(p)=='e'||p_peek(p)=='E') {
        p->pos++;if(p_peek(p)=='+'||p_peek(p)=='-')p->pos++;
        if(p_peek(p)<'0'||p_peek(p)>'9')goto bad;
        while(p->pos<p->len && isdigit((unsigned char)p->s[p->pos]))p->pos++;
    }
    size_t n=p->pos-start;if(n>=64)goto bad;
    char tmp[64];memcpy(tmp,p->s+start,n);tmp[n]=0;
    double number=strtod(tmp,NULL);if(!isfinite(number))goto bad;
    JsonValue *v=json_alloc(JSON_NUMBER);if(!v)return NULL;
    v->as.number=number;return v;
bad:
    p_set_error(p,"numero JSON invalido");return NULL;
}

static JsonValue *p_parse_array(Parser *p) {
    p->pos++; /* consome '[' */
    JsonValue *v = json_alloc(JSON_ARRAY);
    if (!v) return NULL;
    size_t cap = 8;
    v->as.array.items = (JsonValue **)malloc(cap * sizeof(JsonValue *));
    v->as.array.count = 0;

    p_skip_ws(p);
    if (p_peek(p) == ']') { p->pos++; return v; }

    while (1) {
        p_skip_ws(p);
        JsonValue *item = p_parse_value(p);
        if (!item) { json_free(v); return NULL; }
        if (v->as.array.count >= cap) {
            cap *= 2;
            v->as.array.items = (JsonValue **)realloc(v->as.array.items, cap * sizeof(JsonValue *));
        }
        v->as.array.items[v->as.array.count++] = item;
        p_skip_ws(p);
        int c = p_peek(p);
        if (c == ',') { p->pos++; continue; }
        if (c == ']') { p->pos++; break; }
        p_set_error(p, "esperado ',' ou ']' em array");
        json_free(v);
        return NULL;
    }
    return v;
}

static JsonValue *p_parse_object(Parser *p) {
    p->pos++; /* consome '{' */
    JsonValue *v = json_alloc(JSON_OBJECT);
    if (!v) return NULL;
    size_t cap = 8;
    v->as.object.keys = (char **)malloc(cap * sizeof(char *));
    v->as.object.values = (JsonValue **)malloc(cap * sizeof(JsonValue *));
    v->as.object.count = 0;

    p_skip_ws(p);
    if (p_peek(p) == '}') { p->pos++; return v; }

    while (1) {
        p_skip_ws(p);
        char *key = NULL;
        if (!p_parse_string_raw(p, &key)) { json_free(v); return NULL; }
        p_skip_ws(p);
        if (p_peek(p) != ':') { p_set_error(p, "esperado ':' em objeto"); free(key); json_free(v); return NULL; }
        p->pos++;
        p_skip_ws(p);
        JsonValue *val = p_parse_value(p);
        if (!val) { free(key); json_free(v); return NULL; }

        if (v->as.object.count >= cap) {
            cap *= 2;
            v->as.object.keys = (char **)realloc(v->as.object.keys, cap * sizeof(char *));
            v->as.object.values = (JsonValue **)realloc(v->as.object.values, cap * sizeof(JsonValue *));
        }
        v->as.object.keys[v->as.object.count] = key;
        v->as.object.values[v->as.object.count] = val;
        v->as.object.count++;

        p_skip_ws(p);
        int c = p_peek(p);
        if (c == ',') { p->pos++; continue; }
        if (c == '}') { p->pos++; break; }
        p_set_error(p, "esperado ',' ou '}' em objeto");
        json_free(v);
        return NULL;
    }
    return v;
}

static JsonValue *p_parse_value(Parser *p) {
    p_skip_ws(p);
    int c = p_peek(p);
    if (c == -1) { p_set_error(p, "fim inesperado da entrada"); return NULL; }
    if (c == '{' || c == '[') {
        if (++p->depth > 48) { p_set_error(p, "JSON muito profundo"); return NULL; }
        JsonValue *v = c == '{' ? p_parse_object(p) : p_parse_array(p);
        p->depth--; return v;
    }
    if (c == '"') return p_parse_string(p);
    if (c == 't') { JsonValue *v = json_alloc(JSON_BOOL); if (v) v->as.boolean = 1; return p_parse_literal(p, "true", v); }
    if (c == 'f') { JsonValue *v = json_alloc(JSON_BOOL); if (v) v->as.boolean = 0; return p_parse_literal(p, "false", v); }
    if (c == 'n') { JsonValue *v = json_alloc(JSON_NULL); return p_parse_literal(p, "null", v); }
    if (c == '-' || isdigit(c)) return p_parse_number(p);
    p_set_error(p, "token inesperado");
    return NULL;
}

JsonValue *json_parse(const char *text, char *errbuf, size_t errbuf_size) {
    Parser p;
    p.s = text;
    p.pos = 0;
    p.len = strlen(text);
    p.errbuf = errbuf;
    p.errbuf_size = errbuf_size;
    p.failed = 0;
    p.depth = 0;

    JsonValue *v = p_parse_value(&p);
    if (!v) return NULL;
    p_skip_ws(&p);
    if (p.pos != p.len) {
        json_free(v);
        p_set_error(&p, "lixo apos o valor JSON principal");
        return NULL;
    }
    return v;
}

void json_free(JsonValue *v) {
    if (!v) return;
    switch (v->type) {
        case JSON_STRING:
            free(v->as.string);
            break;
        case JSON_ARRAY:
            for (size_t i = 0; i < v->as.array.count; i++) json_free(v->as.array.items[i]);
            free(v->as.array.items);
            break;
        case JSON_OBJECT:
            for (size_t i = 0; i < v->as.object.count; i++) {
                free(v->as.object.keys[i]);
                json_free(v->as.object.values[i]);
            }
            free(v->as.object.keys);
            free(v->as.object.values);
            break;
        default:
            break;
    }
    free(v);
}

const JsonValue *json_object_get(const JsonValue *obj, const char *key) {
    if (!obj || obj->type != JSON_OBJECT) return NULL;
    for (size_t i = 0; i < obj->as.object.count; i++) {
        if (strcmp(obj->as.object.keys[i], key) == 0) return obj->as.object.values[i];
    }
    return NULL;
}

long long json_get_int(const JsonValue *obj, const char *key, long long def) {
    const JsonValue *v = json_object_get(obj, key);
    if (!v || v->type != JSON_NUMBER || !isfinite(v->as.number) || v->as.number < -9007199254740991.0 || v->as.number > 9007199254740991.0) return def;
    return (long long)v->as.number;
}

int json_get_bool(const JsonValue *obj, const char *key, int def) {
    const JsonValue *v = json_object_get(obj, key);
    if (!v || v->type != JSON_BOOL) return def;
    return v->as.boolean;
}

const char *json_get_string(const JsonValue *obj, const char *key, const char *def) {
    const JsonValue *v = json_object_get(obj, key);
    if (!v || v->type != JSON_STRING) return def;
    return v->as.string;
}

const JsonValue *json_get_array(const JsonValue *obj, const char *key) {
    const JsonValue *v = json_object_get(obj, key);
    if (!v || v->type != JSON_ARRAY) return NULL;
    return v;
}

size_t json_array_length(const JsonValue *arr) {
    if (!arr || arr->type != JSON_ARRAY) return 0;
    return arr->as.array.count;
}

long long json_array_get_int(const JsonValue *arr, size_t index, long long def) {
    if (!arr || arr->type != JSON_ARRAY || index >= arr->as.array.count) return def;
    JsonValue *v = arr->as.array.items[index];
    if (v->type != JSON_NUMBER || !isfinite(v->as.number) || v->as.number < -9007199254740991.0 || v->as.number > 9007199254740991.0) return def;
    return (long long)v->as.number;
}

/* ============================= WRITER ============================= */

void jw_init(JsonWriter *w) {
    w->cap = 4096;
    w->len = 0;
    w->buf = (char *)malloc(w->cap);
    w->buf[0] = '\0';
    w->depth = 0;
    memset(w->wrote_item, 0, sizeof(w->wrote_item));
}

void jw_free(JsonWriter *w) {
    free(w->buf);
    w->buf = NULL;
}

char *jw_take(JsonWriter *w) {
    char *b = w->buf;
    w->buf = NULL;
    return b;
}

static void jw_ensure(JsonWriter *w, size_t extra) {
    if (w->len + extra + 1 > w->cap) {
        while (w->len + extra + 1 > w->cap) w->cap *= 2;
        w->buf = (char *)realloc(w->buf, w->cap);
    }
}

static void jw_raw(JsonWriter *w, const char *s) {
    size_t n = strlen(s);
    jw_ensure(w, n);
    memcpy(w->buf + w->len, s, n);
    w->len += n;
    w->buf[w->len] = '\0';
}

static void jw_maybe_comma(JsonWriter *w) {
    if (w->depth > 0 && w->wrote_item[w->depth]) {
        jw_raw(w, ",");
    }
    if (w->depth > 0) w->wrote_item[w->depth] = 1;
}

void jw_obj_start(JsonWriter *w) {
    jw_maybe_comma(w);
    jw_raw(w, "{");
    w->depth++;
    w->wrote_item[w->depth] = 0;
}

void jw_obj_end(JsonWriter *w) {
    w->depth--;
    jw_raw(w, "}");
}

void jw_arr_start(JsonWriter *w) {
    jw_maybe_comma(w);
    jw_raw(w, "[");
    w->depth++;
    w->wrote_item[w->depth] = 0;
}

void jw_arr_end(JsonWriter *w) {
    w->depth--;
    jw_raw(w, "]");
}

void jw_key(JsonWriter *w, const char *key) {
    jw_maybe_comma(w);
    jw_raw(w, "\"");
    jw_raw(w, key);
    jw_raw(w, "\":");
    /* a proxima escrita (valor) nao deve inserir virgula antes de si mesma */
    w->wrote_item[w->depth] = 0;
}

static void jw_write_escaped(JsonWriter *w, const char *s) {
    jw_raw(w, "\"");
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '"': jw_raw(w, "\\\""); break;
            case '\\': jw_raw(w, "\\\\"); break;
            case '\n': jw_raw(w, "\\n"); break;
            case '\t': jw_raw(w, "\\t"); break;
            case '\r': jw_raw(w, "\\r"); break;
            default: {
                char tmp[2] = { *p, 0 };
                jw_raw(w, tmp);
            }
        }
    }
    jw_raw(w, "\"");
}

void jw_str(JsonWriter *w, const char *s) {
    jw_maybe_comma(w);
    jw_write_escaped(w, s);
}

void jw_int(JsonWriter *w, long long v) {
    jw_maybe_comma(w);
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "%lld", v);
    jw_raw(w, tmp);
}

void jw_bool(JsonWriter *w, int v) {
    jw_maybe_comma(w);
    jw_raw(w, v ? "true" : "false");
}

void jw_null(JsonWriter *w) {
    jw_maybe_comma(w);
    jw_raw(w, "null");
}

void jw_key_int(JsonWriter *w, const char *key, long long v) {
    jw_key(w, key);
    jw_int(w, v);
}

void jw_key_bool(JsonWriter *w, const char *key, int v) {
    jw_key(w, key);
    jw_bool(w, v);
}

void jw_key_str(JsonWriter *w, const char *key, const char *v) {
    jw_key(w, key);
    jw_str(w, v);
}
