// Código do simulador
// melhor achar um nome mais sofisticado

#include <stdint.h>
#include <string.h>

#define MEMO_SIZE 4096

//1: representar os componentes do mic1

typedef struct componentesMIC1 {
  uint16_t PC;
  int16_t AC;  //AC pode ficar neg por causa do jneg
  uint16_t SP;
  uint16_t memory[MEMO_SIZE];
  int running;
}MIC1;


//starta zerado
void mic1_init(MIC1 *m) {
    m->PC = 0;
    m->AC = 0;
    m->SP = 0;
    m->running = 0;
    for (int i = 0; i < MEMO_SIZE; i++) {
        m->memory[i] = 0;
    }
}

//2: uma instrução por vez! (von niuma)
void mic1_step(MIC1 *m) {
    // 1. FETCH: pega o numero de 16 bits que está na pos atual da memoria
    uint16_t instruction = m->memory[m->PC];
    m->PC++;

    // 2. DECODE: extrai o opcode (4 bits mais significativos) e o operando (12 bits restantes)
    uint8_t opcode = (instruction >> 12) & 0xF; // >> empurra os bits a direita. ex: 0111 0000 0000 0101 fica 0111. & 0xF garante que nao sobre nada dps os 4 primeiros bits
    uint16_t operando = instruction & 0x0FFF; // sobra apenas o operando. 0x0FFF remove o opcode.

    // 3. EXECUTE
    switch (opcode) {
        case 0b0000: // LODD
            m->AC = m->memory[operando];
            break;
        case 0b0001: // STOD
            m->memory[operando] = m->AC;
            break;
        case 0b0010: // ADDD
            m->AC += m->memory[operando];
            break;
        case 0b0100: // JPOS
            if (m->AC >= 0) m->PC = operando;
            break;
        case 0b0101: // JZER
            if (m->AC == 0) m->PC = operando;
            break;
        case 0b0110: // JUMP
            m->PC = operando;
            break;
        case 0b0111: // LOCO
            m->AC = operando;
            break;
        case 0b1000: // LODL
            m->AC = m->memory[m->SP + operando];
            break;
        case 0b1001: // STOL
            m->memory[m->SP + operando] = m->AC;
            break;
        case 0b1010: // ADDL
            m->AC += m->memory[m->SP + operando];
            break;
        case 0b1011: // SUBL
            m->AC -= m->memory[m->SP + operando];
            break;
        case 0b1100: // JNEG
            if (m->AC < 0) m->PC = operando;
            break;
        case 0b1101: // JNZE
            if (m->AC != 0) m->PC = operando;
            break;
        case 0b1110: // CALL
            m->SP--;
            m->memory[m->SP] = m->PC;
            m->PC = operando;
            break;
        case 0b1111:
            // instruction estendida - falta resolver 
            printf("instruction estendida ainda nao implementada\n");
            m->running = 0;
            break;
        default:
            m->running = 0;
            break;
    }
}


