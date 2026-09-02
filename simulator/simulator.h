// Definições de funções e tipos, somente

#ifndef SIMULATOR_H //evita duplicação de definições quando der include em outros codigos (if not defined ifndef)
#define SIMULATOR_H

#include <stdint.h>

#define MEMO_SIZE 4096

typedef struct componentesMIC1 {
    uint16_t PC;
    int16_t AC;  // AC pode ficar neg por causa do jneg
    uint16_t SP;
    uint16_t memory[MEMO_SIZE];
    int running;
} MIC1;

void mic1_init(MIC1 *m);
void mic1_step(MIC1 *m);

#endif
