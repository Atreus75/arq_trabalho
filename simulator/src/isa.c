#include "mic1.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ---------------------------------------------------------------------
 * Tabela oficial do ISA MIC-1 (Tanenbaum). A ordem dos campos aqui NAO
 * pode mudar sem atualizar mic1_encode/mic1_decode, que usam a posicao
 * numerica do enum Mic1Opcode para calcular os bits diretamente.
 * ------------------------------------------------------------------- */

// x -> operando a seguir pode ser no maximo de 12 bits (0 a 4095 bits)
// y -> 0 a 255, 8 bits
// c -> endereço de chamada, de 0 a 1023
// n -> não aceita operando


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
    return MIC1_ISA_TABLE[op].mnemonico;
}

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
 * ------------------------------------------------------------------- */
void mic1_decode(uint16_t palavra, Mic1Opcode *opSaida, int *xSaida, int *ySaida) {
    int opcode4 = (palavra >> 12) & 0xF;
    *xSaida = palavra & MIC1_ADDR_MASK;
    *ySaida = palavra & 0x00FF;

    if ((palavra & 0xFC00u) == 0xE000u) {
        *opSaida = OP_CALL;
        *xSaida = palavra & 0x03FFu;
        *ySaida = palavra & 0x00FFu;
        return;
    }
    if (opcode4 < 0xE) {
        *opSaida = (Mic1Opcode)opcode4; /* enum LODD..JNZE == 0..13, mapeamento direto */
        return;
    }
    /* Bits exatos da tabela: INSP=11111100, DESP=11111110.
     * Os seis primeiros estendidos nao aceitam bits extras. */
    if ((palavra & 0xFF00u) == 0xFC00u) { *opSaida = OP_INSP; return; }
    if ((palavra & 0xFF00u) == 0xFE00u) { *opSaida = OP_DESP; return; }
    switch (palavra) {
        case 0xF000: *opSaida = OP_PSHI; return;
        case 0xF200: *opSaida = OP_POPI; return;
        case 0xF400: *opSaida = OP_PUSH; return;
        case 0xF600: *opSaida = OP_POP; return;
        case 0xF800: *opSaida = OP_RETN; return;
        case 0xFA00: *opSaida = OP_SWAP; return;
        default: *opSaida = OP_INVALID; return;
    }
}

int mic1_encode(const char *mnemonico, long operando, uint16_t *palavraSaida, char *msgErro, size_t tamErro) {
    char maiusculo[16];
    size_t n = strlen(mnemonico);
    if (n >= sizeof(maiusculo)) n = sizeof(maiusculo) - 1;
    for (size_t i = 0; i < n; i++) maiusculo[i] = (char)toupper((unsigned char)mnemonico[i]);
    maiusculo[n] = '\0';

    Mic1Opcode op = OP_INVALID;
    char tipo = 'n';
    for (int i = 0; i < OP_COUNT; i++) {
        if (strcmp(MIC1_ISA_TABLE[i].mnemonico, maiusculo) == 0) {
            op = MIC1_ISA_TABLE[i].op;
            tipo = MIC1_ISA_TABLE[i].tipoOperando;
            break;
        }
    }
    if (op == OP_INVALID) {
        snprintf(msgErro, tamErro, "mnemonico desconhecido: '%s'", mnemonico);
        return -1;
    }

    uint16_t palavra = 0;
    if (op == OP_CALL) {
        if (operando < 0 || operando > 1023) {
            snprintf(msgErro, tamErro, "operando de CALL deve estar entre 0 e 1023 (recebido %ld)", operando);
            return -1;
        }
        palavra = (uint16_t)(0xE000u | (operando & 0x03FFu));
    } else if (tipo == 'x') {
        if (operando < 0 || operando > 4095) {
            snprintf(msgErro, tamErro, "operando de %s deve estar entre 0 e 4095 (recebido %ld)", maiusculo, operando);
            return -1;
        }
        int opcode4 = (int)op; /* 0..13 */
        palavra = (uint16_t)(((opcode4 & 0xF) << 12) | (operando & MIC1_ADDR_MASK));
    } else if (tipo == 'y') {
        if (operando < 0 || operando > 255) {
            snprintf(msgErro, tamErro, "operando de %s deve estar entre 0 e 255 (recebido %ld)", maiusculo, operando);
            return -1;
        }
        palavra = (uint16_t)((op == OP_DESP ? 0xFE00u : 0xFC00u) | (operando & 0xFF));
    } else { /* 'n' - sem operando */
        if (operando != 0) {
            snprintf(msgErro, tamErro, "%s nao aceita operando", maiusculo);
            return -1;
        }
        int sub3 = (int)(op - OP_PSHI); /* 0..5 */
        palavra = (uint16_t)(0xF000u | ((unsigned)(sub3 & 0x7) << 9));
    }
    *palavraSaida = palavra;
    return 0;
}

