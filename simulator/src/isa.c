#include "mic1.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ---------------------------------------------------------------------
 * Tabela oficial do ISA MIC-1 (Tanenbaum). A ordem dos campos aqui NAO
 * pode mudar sem atualizar mic1_encode/mic1_decode, que usam a posicao
 * numerica do enum Mic1Opcode para calcular os bits diretamente.
 * ------------------------------------------------------------------- */
const Mic1InstrInfo MIC1_ISA_TABLE[OP_COUNT] = {
    { OP_LODD, "LODD", "ac := m[x]",                              'x' },
    { OP_STOD, "STOD", "m[x] := ac",                               'x' },
    { OP_ADDD, "ADDD", "ac := ac + m[x]",                          'x' },
    { OP_SUBD, "SUBD", "ac := ac - m[x]",                          'x' },
    { OP_JPOS, "JPOS", "if ac >= 0 then pc := x",                  'x' },
    { OP_JZER, "JZER", "if ac = 0 then pc := x",                   'x' },
    { OP_JUMP, "JUMP", "pc := x",                                  'x' },
    { OP_LOCO, "LOCO", "ac := x (0 <= x <= 4095)",                 'x' },
    { OP_LODL, "LODL", "ac := m[sp + x]",                          'x' },
    { OP_STOL, "STOL", "m[x + sp] := ac",                          'x' },
    { OP_ADDL, "ADDL", "ac := ac + m[sp + x]",                     'x' },
    { OP_SUBL, "SUBL", "ac := ac - m[sp + x]",                     'x' },
    { OP_JNEG, "JNEG", "if ac < 0 then pc := x",                   'x' },
    { OP_JNZE, "JNZE", "if ac != 0 then pc := x",                  'x' },
    { OP_CALL, "CALL", "sp := sp - 1; m[sp] := pc; pc := x",       'c' },
    { OP_PSHI, "PSHI", "sp := sp - 1; m[sp] := m[ac]",             'n' },
    { OP_POPI, "POPI", "m[ac] := m[sp]; sp := sp + 1",             'n' },
    { OP_PUSH, "PUSH", "sp := sp - 1; m[sp] := ac",                'n' },
    { OP_POP,  "POP",  "ac := m[sp]; sp := sp + 1",                'n' },
    { OP_RETN, "RETN", "pc := m[sp]; sp := sp + 1",                'n' },
    { OP_SWAP, "SWAP", "tmp := ac; ac := sp; sp := tmp",           'n' },
    { OP_INSP, "INSP", "sp := sp + y (0 <= y <= 255)",             'y' },
    { OP_DESP, "DESP", "sp := sp - y (0 <= y <= 255)",             'y' },
};

const char *mic1_mnemonic_name(Mic1Opcode op) {
    if (op < 0 || op >= OP_COUNT) return "???";
    return MIC1_ISA_TABLE[op].mnemonic;
}

/* ---------------------------------------------------------------------
 * Helpers de aritmetica segura (evitam undefined behavior de overflow
 * com inteiros com sinal, fazendo a conta em 32 bits e truncando).
 * ------------------------------------------------------------------- */
/* ---------------------------------------------------------------------
 * Decode / Encode
 *
 * Layout de bits (palavra de 16 bits):
 *   opcode4 = bits[15:12]
 *   0000..1110 (0-14)  -> LODD..CALL, x = bits[11:0]  (endereco/constante 12 bits)
 *   1111               -> grupo estendido:
 *       sub3 = bits[11:9]
 *       000 PSHI  001 POPI  010 PUSH  011 POP  100 RETN  101 SWAP
 *       110, bit8=0 -> INSP; 111, bit8=0 -> DESP; y = bits[7:0]
 *       HALT=0xFF00 e uma extensao explicita do simulador
 * ------------------------------------------------------------------- */
void mic1_decode(uint16_t word, Mic1Opcode *outOp, int *outX, int *outY) {
    int opcode4 = (word >> 12) & 0xF;
    *outX = word & MIC1_ADDR_MASK;
    *outY = word & 0x00FF;

    if ((word & 0xFC00u) == 0xE000u) {
        *outOp = OP_CALL;
        *outX = word & 0x03FFu;
        *outY = word & 0x00FFu;
        return;
    }
    if (opcode4 < 0xE) {
        *outOp = (Mic1Opcode)opcode4; /* enum LODD..JNZE == 0..13, mapeamento direto */
        return;
    }
    /* Bits exatos da tabela: INSP=11111100, DESP=11111110.
     * Os seis primeiros estendidos nao aceitam bits extras. */
    if ((word & 0xFF00u) == 0xFC00u) { *outOp = OP_INSP; return; }
    if ((word & 0xFF00u) == 0xFE00u) { *outOp = OP_DESP; return; }
    switch (word) {
        case 0xF000: *outOp = OP_PSHI; return;
        case 0xF200: *outOp = OP_POPI; return;
        case 0xF400: *outOp = OP_PUSH; return;
        case 0xF600: *outOp = OP_POP; return;
        case 0xF800: *outOp = OP_RETN; return;
        case 0xFA00: *outOp = OP_SWAP; return;
        default: *outOp = OP_INVALID; return;
    }
}

int mic1_encode(const char *mnemonic, long operand, uint16_t *outWord, char *errMsg, size_t errMsgSize) {
    char upper[16];
    size_t n = strlen(mnemonic);
    if (n >= sizeof(upper)) n = sizeof(upper) - 1;
    for (size_t i = 0; i < n; i++) upper[i] = (char)toupper((unsigned char)mnemonic[i]);
    upper[n] = '\0';

    Mic1Opcode op = OP_INVALID;
    char kind = 'n';
    for (int i = 0; i < OP_COUNT; i++) {
        if (strcmp(MIC1_ISA_TABLE[i].mnemonic, upper) == 0) {
            op = MIC1_ISA_TABLE[i].op;
            kind = MIC1_ISA_TABLE[i].operandKind;
            break;
        }
    }
    if (op == OP_INVALID) {
        snprintf(errMsg, errMsgSize, "mnemonico desconhecido: '%s'", mnemonic);
        return -1;
    }

    uint16_t word = 0;
    if (op == OP_CALL) {
        if (operand < 0 || operand > 1023) {
            snprintf(errMsg, errMsgSize, "operando de CALL deve estar entre 0 e 1023 (recebido %ld)", operand);
            return -1;
        }
        word = (uint16_t)(0xE000u | (operand & 0x03FFu));
    } else if (kind == 'x') {
        if (operand < 0 || operand > 4095) {
            snprintf(errMsg, errMsgSize, "operando de %s deve estar entre 0 e 4095 (recebido %ld)", upper, operand);
            return -1;
        }
        int opcode4 = (int)op; /* 0..13 */
        word = (uint16_t)(((opcode4 & 0xF) << 12) | (operand & MIC1_ADDR_MASK));
    } else if (kind == 'y') {
        if (operand < 0 || operand > 255) {
            snprintf(errMsg, errMsgSize, "operando de %s deve estar entre 0 e 255 (recebido %ld)", upper, operand);
            return -1;
        }
        word = (uint16_t)((op == OP_DESP ? 0xFE00u : 0xFC00u) | (operand & 0xFF));
    } else { /* 'n' - sem operando */
        if (operand != 0) {
            snprintf(errMsg, errMsgSize, "%s nao aceita operando", upper);
            return -1;
        }
        int sub3 = (int)(op - OP_PSHI); /* 0..5 */
        word = (uint16_t)(0xF000u | ((unsigned)(sub3 & 0x7) << 9));
    }
    *outWord = word;
    return 0;
}

