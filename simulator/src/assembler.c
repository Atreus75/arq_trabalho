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
    int numero; char *texto; char mnemonico[16], operando[128]; uint16_t palavra;
} Line;
typedef struct {
    Line linhas[MIC1_MEM_SIZE];
    Symbol flags[MAX_SYMBOLS];
    Symbol vars[MAX_SYMBOLS];
    int qtd, flagCount, qtdVars, extra, linhaErro;
    char erro[180];
} Assembly;

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    char *fim = s + strlen(s);
    while (fim > s && isspace((unsigned char)fim[-1])) *--fim = 0;
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

static int lookup_flag(const Assembly *a, const char *nome) {
    for (int i = 0; i < a->flagCount; i++) {
        if (!strcmp(a->flags[i].nome, nome)) return a->flags[i].endereco;
    }
    return -1;
}

static int lookup_var(const Assembly *a, const char *nome) {
    for (int i = 0; i < a->qtdVars; i++) {
        if (!strcmp(a->vars[i].nome, nome)) return a->vars[i].endereco;
    }
    return -1;
}

static void erro(Assembly *a, int linha, const char *msg) {
    if (*a->erro) return;
    a->linhaErro = linha;
    snprintf(a->erro, sizeof(a->erro), "%s", msg);
}

static void add_flag(Assembly *a, const char *nome, int endereco, int linha) {
    if (!identifier(nome)) {
        erro(a, linha, "Identificador invalido (maximo 63 caracteres).");
        return;
    }
    if (lookup_flag(a, nome) >= 0) {
        erro(a, linha, "Rotulo repetido.");
        return;
    }
    if (a->flagCount >= MAX_SYMBOLS) {
        erro(a, linha, "Tabela de rotulos cheia.");
        return;
    }
    Symbol *s = &a->flags[a->flagCount++];
    snprintf(s->nome, sizeof(s->nome), "%s", nome);
    s->endereco = endereco;
}

static int numero(const char *s, long *saida) {
    if (!s || !*s) return 0;
    char *fim; errno = 0; long valor = strtol(s, &fim, 10);
    if (errno || *fim) return 0;
    *saida = valor; return 1;
}

static int opcode(const char *nome) {
    for (int i = 0; i < OP_COUNT; i++) {
        if (!strcmp(nome, MIC1_ISA_TABLE[i].mnemonico)) return i;
    }
    return -1;
}

static bool is_jump_or_call(int op) {
    return op == OP_JUMP || op == OP_JPOS || op == OP_JZER ||
           op == OP_JNEG || op == OP_JNZE || op == OP_CALL;
}

static void word_json(JsonWriter *w, int endereco, const Line *l) {
    char bits[17];
    for (int i = 0; i < 16; i++) bits[i] = (char)('0' + ((l->palavra >> (15 - i)) & 1));
    bits[16] = 0;
    jw_obj_start(w);
    jw_key_int(w, "address", endereco);
    jw_key_int(w, "word", l->palavra);
    jw_key_str(w, "binaryString", bits);
    jw_key_str(w, "mnemonic", l->mnemonico);
    jw_key_str(w, "operandDisplay", l->operando);
    jw_key_int(w, "sourceLine", l->numero);
    jw_key_str(w, "sourceText", l->texto ? l->texto : "");
    jw_obj_end(w);
}

char *assemble_json(const char *codigo, const char *salvoComo) {
    Assembly *a = calloc(1, sizeof(*a));
    if (!a) return NULL;
    size_t tam = strlen(codigo);
    char *copia = malloc(tam + 1);
    if (!copia) { free(a); return NULL; }
    memcpy(copia, codigo, tam + 1);
    char *cursor = copia; int numLinha = 0;

    /* Passada 1: enderecos de instrucoes e rotulos (flags). */
    while (*cursor && !*a->erro) {
        char *bruto = cursor; char *fim = strpbrk(cursor, "\r\n");
        if (fim) {
            char sep = *fim; *fim = 0; cursor = fim + 1;
            if (sep == '\r' && *cursor == '\n') cursor++;
        } else {
            cursor += strlen(cursor);
        }
        if (++numLinha > MAX_LINES) {
            erro(a, numLinha, "Limite de 8192 linhas excedido.");
            break;
        }
        char *comentario = strchr(bruto, ';'), *barra = strstr(bruto, "//");
        if (barra && (!comentario || barra < comentario)) comentario = barra;
        if (comentario) *comentario = 0;
        char *linha = trim(bruto);
        if (!*linha) continue;

        char *doisPontos;
        while ((doisPontos = strchr(linha, ':')) != NULL) {
            *doisPontos = 0;
            char *rotulo = trim(linha);
            add_flag(a, rotulo, a->qtd, numLinha);
            linha = trim(doisPontos + 1);
            if (*a->erro) break;
        }
        if (!*linha || *a->erro) continue;
        if (a->qtd >= MIC1_MEM_SIZE) {
            erro(a, numLinha, "Programa excede 4096 palavras.");
            break;
        }

        Line *l = &a->linhas[a->qtd];
        l->numero = numLinha;
        l->texto = linha;

        /* Palavra binaria bruta de 16 bits */
        if (strlen(linha) == 16 && strspn(linha, "01") == 16) {
            uint16_t val = 0;
            for (int k = 0; k < 16; k++) val = (uint16_t)((val << 1) | (unsigned)(linha[k] - '0'));
            l->palavra = val;
            Mic1Opcode op;
            int x, y;
            mic1_decode(val, &op, &x, &y);
            if (op != OP_INVALID && op < OP_COUNT) {
                strncpy(l->mnemonico, MIC1_ISA_TABLE[op].mnemonico, sizeof(l->mnemonico) - 1);
                l->mnemonico[sizeof(l->mnemonico) - 1] = '\0';
                if (MIC1_ISA_TABLE[op].tipoOperando == 'x') snprintf(l->operando, sizeof(l->operando), "%d", x);
                else if (MIC1_ISA_TABLE[op].tipoOperando == 'y') snprintf(l->operando, sizeof(l->operando), "%d", y);
                else snprintf(l->operando, sizeof(l->operando), "(sem operando)");
            } else {
                l->mnemonico[0] = '\0';
                snprintf(l->operando, sizeof(l->operando), "(sem operando)");
            }
            a->qtd++;
            continue;
        }

        char mn[1024], arg[1024], sobra[2];
        if (strlen(linha) >= 1024) {
            erro(a, numLinha, "Linha muito longa (maximo 1023 caracteres).");
            break;
        }
        int tokens = sscanf(linha, "%1023s %1023s %1s", mn, arg, sobra);
        if (tokens > 2 || strlen(mn) >= sizeof(l->mnemonico) || (tokens == 2 && strlen(arg) >= sizeof(l->operando))) {
            erro(a, numLinha, "Instrucao ou operando invalido; use uma instrucao por linha.");
            break;
        }
        for (size_t i = 0; i <= strlen(mn); i++) {
            l->mnemonico[i] = (char)toupper((unsigned char)mn[i]);
        }
        if (tokens == 2) {
            strcpy(l->operando, arg);
        }
        a->qtd++;
    }
    if (!a->qtd && !*a->erro) erro(a, 0, "Escreva ao menos uma instrucao.");

    /* Passada 2: codificacao e resolucao de simbolos (flags e vars). */
    for (int i = 0; i < a->qtd && !*a->erro; i++) {
        Line *l = &a->linhas[i];
        if (strlen(l->texto) == 16 && strspn(l->texto, "01") == 16) {
            continue; /* Palavra binaria bruta ja codificada */
        }
        int op = opcode(l->mnemonico);
        if (op < 0) {
            erro(a, l->numero, "Mnemonico desconhecido.");
            continue;
        }
        char tipo = MIC1_ISA_TABLE[op].tipoOperando;
        if (tipo == 'n') {
            if (*l->operando) {
                erro(a, l->numero, "Esta instrucao nao aceita operando.");
                continue;
            }
            char msg[160];
            if (mic1_encode(l->mnemonico, 0, &l->palavra, msg, sizeof(msg))) {
                erro(a, l->numero, msg);
            }
            snprintf(l->operando, sizeof(l->operando), "(sem operando)");
            continue;
        }

        if (!*l->operando) {
            erro(a, l->numero, "Falta o operando.");
            continue;
        }

        char orig[64];
        snprintf(orig, sizeof(orig), "%s", l->operando);
        bool ehSimbolico = false;
        long valor = 0;
        if (is_jump_or_call(op)) {
            long valNum = 0;
            if (numero(l->operando, &valNum)) {
                if (op == OP_CALL) {
                    if (valNum < 0 || valNum >= 1024) {
                        erro(a, l->numero, "CALL nao suporta endereco >= 1024.");
                        continue;
                    }
                } else {
                    if (valNum < 0 || valNum > 4095) {
                        erro(a, l->numero, "Endereco de salto deve estar entre 0 e 4095.");
                        continue;
                    }
                }
                valor = valNum;
            } else {
                if (!identifier(l->operando)) {
                    erro(a, l->numero, "Identificador invalido.");
                    continue;
                }
                int end = lookup_flag(a, l->operando);
                if (end < 0) {
                    char msg[160];
                    snprintf(msg, sizeof(msg), "Rotulo nao declarado para salto: %s", l->operando);
                    erro(a, l->numero, msg);
                    continue;
                }
                if (op == OP_CALL && end >= 1024) {
                    erro(a, l->numero, "CALL nao suporta endereco >= 1024.");
                    continue;
                }
                valor = end;
                ehSimbolico = true;
            }
        } else {
            long valNum = 0;
            if (numero(l->operando, &valNum)) {
                valor = valNum;
            } else {
                if (!identifier(l->operando)) {
                    erro(a, l->numero, "Identificador invalido ou operando numerico invalido.");
                    continue;
                }
                int end = lookup_var(a, l->operando);
                if (end >= 0) {
                    valor = end;
                } else {
                    if (a->qtd + a->extra >= MIC1_MEM_SIZE || a->qtdVars >= MAX_SYMBOLS) {
                        erro(a, l->numero, "Memoria ou tabela de variaveis cheia.");
                        continue;
                    }
                    end = a->qtd + a->extra++;
                    Symbol *v = &a->vars[a->qtdVars++];
                    snprintf(v->nome, sizeof(v->nome), "%s", l->operando);
                    v->endereco = end;
                    valor = end;
                }
                ehSimbolico = true;
            }
        }

        if (!*a->erro) {
            char msg[160];
            if (mic1_encode(l->mnemonico, valor, &l->palavra, msg, sizeof(msg))) {
                erro(a, l->numero, msg);
            }
            if (ehSimbolico) {
                snprintf(l->operando, sizeof(l->operando), "%ld (label '%s')", valor, orig);
            } else {
                snprintf(l->operando, sizeof(l->operando), "%ld", valor);
            }
        }
    }

    JsonWriter w; jw_init(&w); jw_obj_start(&w); jw_key_bool(&w, "success", !*a->erro);
    jw_key(&w, "program"); jw_arr_start(&w);
    if (!*a->erro) {
        for (int i = 0; i < a->qtd; i++) word_json(&w, i, &a->linhas[i]);
    }
    jw_arr_end(&w);
    jw_key(&w, "labelTable"); jw_obj_start(&w);
    for (int i = 0; i < a->flagCount; i++) jw_key_int(&w, a->flags[i].nome, a->flags[i].endereco);
    for (int i = 0; i < a->qtdVars; i++) jw_key_int(&w, a->vars[i].nome, a->vars[i].endereco);
    jw_obj_end(&w);
    jw_key(&w, "errors"); jw_arr_start(&w);
    if (*a->erro) {
        jw_obj_start(&w);
        jw_key_int(&w, "lineNumber", a->linhaErro);
        jw_key_str(&w, "message", a->erro);
        jw_key_str(&w, "sourceText", "");
        jw_obj_end(&w);
    }
    jw_arr_end(&w);
    if (salvoComo) jw_key_str(&w, "savedAs", salvoComo);
    jw_obj_end(&w);
    char *saida = jw_take(&w);
    jw_free(&w);
    free(copia);
    free(a);
    return saida;
}
