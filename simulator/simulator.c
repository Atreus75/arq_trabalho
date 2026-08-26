// Código do simulador
// melhor achar um nome mais sofisticado

#include <stdint.h>
#include <string.h>

#define MEMO_SIZE 4096

//1: representar os componentes do mic1

typedef struct componentesMIC1 {
  u_int16_t PC;
  int16_t AC;  //AC pode ficar neg por causa do jneg
  u_int16_t SP;
  u_int16_t memory[MEMO_SIZE];
  int running;
}MIC1;


//starta zerado
void mic1_init(MIC1 *m) {
    m->PC = 0;
    m->AC = 0;
    m->SP = 0;
    m->running = 0;
    for (int i = 0; i < MEM_SIZE; i++) {
        m->memory[i] = 0;
    }
}


//facilitar a visu de opcodes (nada certo)
typedef enum {
    OP_LODD = 0b0000,
    OP_STOD = 0b0001,
    OP_ADDD = 0b0010,
    // 0b0011 reservado pra instruções estendidas
    OP_JPOS = 0b0100,
    OP_JZER = 0b0101,
    OP_JUMP = 0b0110,
    OP_LOCO = 0b0111,
    OP_LODL = 0b1000,
    OP_STOL = 0b1001,
    OP_ADDL = 0b1010,
    OP_SUBL = 0b1011,
    OP_JNEG = 0b1100,
    OP_JNZE = 0b1101,
    OP_CALL = 0b1110,
    OP_EXTENDED = 0b1111 
} OPCODE;
