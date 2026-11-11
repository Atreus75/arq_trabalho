/* Montador em duas passadas, com separacao estrita entre flags (rotulos
 * de codigo declarados com :) e vars (variaveis implicitas apos o programa).
 * Saltos exigem rotulos declarados em flags (ou literal numerico);
 * instrucoes nao-salto alocam em vars se nao existirem em flags. */
#include "assembler.h"
#include "mic1.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 8192

typedef AssemblerSymbol Symbol;
typedef struct {
    int number; char *text; char mnemonic[16], operand[128]; uint16_t word;
} Line;
typedef struct {
    Line lines[MIC1_MEM_SIZE];
    Symbol flags[MAX_SYMBOLS];
    Symbol vars[MAX_SYMBOLS];
    int count, flagCount, varCount, extra, errorLine;
    char error[180];
} Assembly;

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = 0;
    return s;
}

static int identifier(const char *s) {
    if (!s || !*s || strlen(s) > 63) return 0;
    if (!(isalpha((unsigned char)*s) || *s == '_')) return 0;
    for (const char *p = s + 1; *p; p++) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
    }
    return 1;
}

static int lookup_flag(const Assembly *a, const char *name) {
    for (int i = 0; i < a->flagCount; i++) {
        if (!strcmp(a->flags[i].name, name)) return a->flags[i].address;
    }
    return -1;
}

static int lookup_var(const Assembly *a, const char *name) {
    for (int i = 0; i < a->varCount; i++) {
        if (!strcmp(a->vars[i].name, name)) return a->vars[i].address;
    }
    return -1;
}

static void error(Assembly *a, int line, const char *message) {
    if (*a->error) return;
    a->errorLine = line;
    snprintf(a->error, sizeof(a->error), "%s", message);
}

static void add_flag(Assembly *a, const char *name, int address, int line) {
    if (!identifier(name)) {
        error(a, line, "Identificador invalido (maximo 63 caracteres).");
        return;
    }
    if (lookup_flag(a, name) >= 0) {
        error(a, line, "Rotulo repetido.");
        return;
    }
    if (a->flagCount >= MAX_SYMBOLS) {
        error(a, line, "Tabela de rotulos cheia.");
        return;
    }
    Symbol *s = &a->flags[a->flagCount++];
    snprintf(s->name, sizeof(s->name), "%s", name);
    s->address = address;
}

static int number(const char *s, long *out) {
    if (!s || !*s) return 0;
    char *end; errno = 0; long value = strtol(s, &end, 10);
    if (errno || *end) return 0;
    *out = value; return 1;
}

static int opcode(const char *name) {
    for (int i = 0; i < OP_COUNT; i++) {
        if (!strcmp(name, MIC1_ISA_TABLE[i].mnemonic)) return i;
    }
    return -1;
}

static bool is_jump_or_call(int op) {
    return op == OP_JUMP || op == OP_JPOS || op == OP_JZER ||
           op == OP_JNEG || op == OP_JNZE || op == OP_CALL;
}

static void word_json(JsonWriter *w, int address, const Line *l) {
    char bits[17];
    for (int i = 0; i < 16; i++) bits[i] = (char)('0' + ((l->word >> (15 - i)) & 1));
    bits[16] = 0;
    jw_obj_start(w);
    jw_key_int(w, "address", address);
    jw_key_int(w, "word", l->word);
    jw_key_str(w, "binaryString", bits);
    jw_key_str(w, "mnemonic", l->mnemonic);
    jw_key_str(w, "operandDisplay", l->operand);
    jw_key_int(w, "sourceLine", l->number);
    jw_key_str(w, "sourceText", l->text ? l->text : "");
    jw_obj_end(w);
}

char *assemble_json(const char *source, const char *savedAs) {
    Assembly *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    size_t size = strlen(source);
    char *copy = malloc(size + 1);
    if (!copy) { free(a); return NULL; }
    memcpy(copy, source, size + 1);
    char *cursor = copy; int lineNumber = 0;

    /* Passada 1: enderecos de instrucoes e rotulos (flags). */
    while (*cursor && !*a->error) {
        char *raw = cursor; char *end = strpbrk(cursor, "\r\n");
        if (end) {
            char sep = *end; *end = 0; cursor = end + 1;
            if (sep == '\r' && *cursor == '\n') cursor++;
        } else {
            cursor += strlen(cursor);
        }
        if (++lineNumber > MAX_LINES) {
            error(a, lineNumber, "Limite de 8192 linhas excedido.");
            break;
        }
        char *comment = strchr(raw, ';'), *slash = strstr(raw, "//");
        if (slash && (!comment || slash < comment)) comment = slash;
        if (comment) *comment = 0;
        char *line = trim(raw);
        if (!*line) continue;

        char *colon;
        while ((colon = strchr(line, ':')) != NULL) {
            *colon = 0;
            char *lbl = trim(line);
            add_flag(a, lbl, a->count, lineNumber);
            line = trim(colon + 1);
            if (*a->error) break;
        }
        if (!*line || *a->error) continue;
        if (a->count >= MIC1_MEM_SIZE) {
            error(a, lineNumber, "Programa excede 4096 palavras.");
            break;
        }

        Line *l = &a->lines[a->count];
        l->number = lineNumber;
        l->text = line;

        /* Palavra binaria bruta de 16 bits */
        if (strlen(line) == 16 && strspn(line, "01") == 16) {
            uint16_t val = 0;
            for (int k = 0; k < 16; k++) val = (uint16_t)((val << 1) | (unsigned)(line[k] - '0'));
            l->word = val;
            Mic1Opcode op;
            int x, y;
            mic1_decode(val, &op, &x, &y);
            if (op != OP_INVALID && op < OP_COUNT) {
                strncpy(l->mnemonic, MIC1_ISA_TABLE[op].mnemonic, sizeof(l->mnemonic) - 1);
                l->mnemonic[sizeof(l->mnemonic) - 1] = '\0';
                if (MIC1_ISA_TABLE[op].operandKind == 'x') snprintf(l->operand, sizeof(l->operand), "%d", x);
                else if (MIC1_ISA_TABLE[op].operandKind == 'y') snprintf(l->operand, sizeof(l->operand), "%d", y);
                else snprintf(l->operand, sizeof(l->operand), "(sem operando)");
            } else {
                l->mnemonic[0] = '\0';
                snprintf(l->operand, sizeof(l->operand), "(sem operando)");
            }
            a->count++;
            continue;
        }

        char mn[1024], arg[1024], extraTok[2];
        if (strlen(line) >= 1024) {
            error(a, lineNumber, "Linha muito longa (maximo 1023 caracteres).");
            break;
        }
        int tokens = sscanf(line, "%1023s %1023s %1s", mn, arg, extraTok);
        if (tokens > 2 || strlen(mn) >= sizeof(l->mnemonic) || (tokens == 2 && strlen(arg) >= sizeof(l->operand))) {
            error(a, lineNumber, "Instrucao ou operando invalido; use uma instrucao por linha.");
            break;
        }
        for (size_t i = 0; i <= strlen(mn); i++) {
            l->mnemonic[i] = (char)toupper((unsigned char)mn[i]);
        }
        if (tokens == 2) {
            strcpy(l->operand, arg);
        }
        a->count++;
    }
    if (!a->count && !*a->error) error(a, 0, "Escreva ao menos uma instrucao.");

    /* Passada 2: codificacao e resolucao de simbolos (flags e vars). */
    for (int i = 0; i < a->count && !*a->error; i++) {
        Line *l = &a->lines[i];
        if (strlen(l->text) == 16 && strspn(l->text, "01") == 16) {
            continue; /* Palavra binaria bruta ja codificada */
        }
        int op = opcode(l->mnemonic);
        if (op < 0) {
            error(a, l->number, "Mnemonico desconhecido.");
            continue;
        }
        char kind = MIC1_ISA_TABLE[op].operandKind;
        if (kind == 'n') {
            if (*l->operand) {
                error(a, l->number, "Esta instrucao nao aceita operando.");
                continue;
            }
            char msg[160];
            if (mic1_encode(l->mnemonic, 0, &l->word, msg, sizeof(msg))) {
                error(a, l->number, msg);
            }
            snprintf(l->operand, sizeof(l->operand), "(sem operando)");
            continue;
        }

        if (!*l->operand) {
            error(a, l->number, "Falta o operando.");
            continue;
        }

        char orig[64];
        snprintf(orig, sizeof(orig), "%s", l->operand);
        bool isSymbolic = false;
        long value = 0;
        if (is_jump_or_call(op)) {
            long numVal = 0;
            if (number(l->operand, &numVal)) {
                if (op == OP_CALL) {
                    if (numVal < 0 || numVal >= 1024) {
                        error(a, l->number, "CALL nao suporta endereco >= 1024.");
                        continue;
                    }
                } else {
                    if (numVal < 0 || numVal > 4095) {
                        error(a, l->number, "Endereco de salto deve estar entre 0 e 4095.");
                        continue;
                    }
                }
                value = numVal;
            } else {
                if (!identifier(l->operand)) {
                    error(a, l->number, "Identificador invalido.");
                    continue;
                }
                int addr = lookup_flag(a, l->operand);
                if (addr < 0) {
                    char msg[160];
                    snprintf(msg, sizeof(msg), "Rotulo nao declarado para salto: %s", l->operand);
                    error(a, l->number, msg);
                    continue;
                }
                if (op == OP_CALL && addr >= 1024) {
                    error(a, l->number, "CALL nao suporta endereco >= 1024.");
                    continue;
                }
                value = addr;
                isSymbolic = true;
            }
        } else {
            long numVal = 0;
            if (number(l->operand, &numVal)) {
                value = numVal;
            } else {
                if (!identifier(l->operand)) {
                    error(a, l->number, "Identificador invalido ou operando numerico invalido.");
                    continue;
                }
                int addr = lookup_var(a, l->operand);
                if (addr >= 0) {
                    value = addr;
                } else {
                    if (a->count + a->extra >= MIC1_MEM_SIZE || a->varCount >= MAX_SYMBOLS) {
                        error(a, l->number, "Memoria ou tabela de variaveis cheia.");
                        continue;
                    }
                    addr = a->count + a->extra++;
                    Symbol *v = &a->vars[a->varCount++];
                    snprintf(v->name, sizeof(v->name), "%s", l->operand);
                    v->address = addr;
                    value = addr;
                }
                isSymbolic = true;
            }
        }

        if (!*a->error) {
            char msg[160];
            if (mic1_encode(l->mnemonic, value, &l->word, msg, sizeof(msg))) {
                error(a, l->number, msg);
            }
            if (isSymbolic) {
                snprintf(l->operand, sizeof(l->operand), "%ld (label '%s')", value, orig);
            } else {
                snprintf(l->operand, sizeof(l->operand), "%ld", value);
            }
        }
    }

    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w, "success", !*a->error);
    jw_key(&w, "program"); jw_arr_start(&w);
    if (!*a->error) {
        for (int i = 0; i < a->count; i++) word_json(&w, i, &a->lines[i]);
    }
    jw_arr_end(&w);
    jw_key(&w, "labelTable"); jw_obj_start(&w);
    for (int i = 0; i < a->flagCount; i++) jw_key_int(&w, a->flags[i].name, a->flags[i].address);
    for (int i = 0; i < a->varCount; i++) jw_key_int(&w, a->vars[i].name, a->vars[i].address);
    jw_obj_end(&w);
    jw_key(&w, "errors"); jw_arr_start(&w);
    if (*a->error) {
        jw_obj_start(&w);
        jw_key_int(&w, "lineNumber", a->errorLine);
        jw_key_str(&w, "message", a->error);
        jw_key_str(&w, "sourceText", "");
        jw_obj_end(&w);
    }
    jw_arr_end(&w);
    if (savedAs) jw_key_str(&w, "savedAs", savedAs);
    jw_obj_end(&w);
    char *out = jw_take(&w);
    jw_free(&w);
    free(copy);
    free(a);
    return out;
}
