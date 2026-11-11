#ifndef MIC1_ASSEMBLER_H
#define MIC1_ASSEMBLER_H
#include "json.h"

#define MAX_SYMBOLS 4096

typedef struct {
    char name[64];
    int address;
} AssemblerSymbol;

/* Resultado compativel com a interface. Toda a montagem acontece em C. */
char *assemble_json(const char *source, const char *savedAs);
#endif
