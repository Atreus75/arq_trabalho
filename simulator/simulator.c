// Código do simulador
// melhor achar um nome mais sofisticado

#include <stdio.h>
#include <string.h>
#include "simulator.h"

//1: representar os componentes do mic1 -> está no .h agora (é uma definição).

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
    uint16_t instruction = m->memory[m->PC];
    m->PC++;

    uint8_t opcode = (instruction >> 12) & 0xF;
    uint16_t operando = instruction & 0x0FFF;

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
        case 0b0011: // SUBD -- esse opcode era vago na tabela do Rodrigo
            m->AC -= m->memory[operando];
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
            printf("instrucao estendida ainda nao implementada\n");
            m->running = 0;
            break;
        default:
            m->running = 0;
            break;
    }
}
