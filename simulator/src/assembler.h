#ifndef MIC1_ASSEMBLER_H
#define MIC1_ASSEMBLER_H
#include "json.h"

#define MAX_SYMBOLS 4096

typedef struct {
    char nome[64];
    int endereco;
} AssemblerSymbol;

/* Resultado compativel com a interface. Toda a montagem acontece em C. */
char *assemble_json(const char *codigo, const char *salvoComo);
#endif
