/*
 * json.h - Mini biblioteca JSON auto-contida (sem dependências externas).
 *
 * Implementa apenas o necessário para o protocolo de comunicação do
 * simulador MIC-1: ler um "pedido" (mode + state) e escrever uma
 * "resposta" (novo state). Não é um parser JSON de propósito geral.
 */
#ifndef MIC1_JSON_H
#define MIC1_JSON_H

#include <stddef.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonValue {
    JsonType type;
    union {
        int boolean;
        double number;
        char *string;
        struct {
            struct JsonValue **items;
            size_t count;
        } array;
        struct {
            char **keys;
            struct JsonValue **values;
            size_t count;
        } object;
    } as;
} JsonValue;

/* Parseia 'text' e retorna a árvore de valores. Em caso de erro retorna
 * NULL e escreve uma mensagem legível em errbuf (se não-NULL). */
JsonValue *json_parse(const char *text, char *errbuf, size_t errbuf_size);
void json_free(JsonValue *v);

/* Acessores de conveniência. Todos toleram NULL / tipo errado retornando
 * o valor default, nunca crasham. */
const JsonValue *json_object_get(const JsonValue *obj, const char *key);
long long json_get_int(const JsonValue *obj, const char *key, long long def);
int json_get_bool(const JsonValue *obj, const char *key, int def);
const char *json_get_string(const JsonValue *obj, const char *key, const char *def);
const JsonValue *json_get_array(const JsonValue *obj, const char *key);

size_t json_array_length(const JsonValue *arr);
long long json_array_get_int(const JsonValue *arr, size_t index, long long def);

/* ---------------- Writer ---------------- */

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    int depth;
    int wrote_item[64]; /* pilha: já escrevemos algo neste nível? */
} JsonWriter;

void jw_init(JsonWriter *w);
void jw_free(JsonWriter *w);
char *jw_take(JsonWriter *w); /* transfere ownership da string final para o chamador */

void jw_obj_start(JsonWriter *w);
void jw_obj_end(JsonWriter *w);
void jw_arr_start(JsonWriter *w);
void jw_arr_end(JsonWriter *w);

void jw_key(JsonWriter *w, const char *key);
void jw_str(JsonWriter *w, const char *s);
void jw_int(JsonWriter *w, long long v);
void jw_bool(JsonWriter *w, int v);
void jw_null(JsonWriter *w);

/* Atalhos "key: value" para objetos */
void jw_key_int(JsonWriter *w, const char *key, long long v);
void jw_key_bool(JsonWriter *w, const char *key, int v);
void jw_key_str(JsonWriter *w, const char *key, const char *v);

#endif /* MIC1_JSON_H */
