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



