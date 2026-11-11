#ifndef MIC1_H
#define MIC1_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cache.h"

#define MIC1_MEM_SIZE 4096
#define MIC1_ADDR_MASK 0x0fff
#define MIC1_ROM_SIZE 256
#define MIC1_MAX_TRACE 160
enum { R_PC, R_AC, R_SP, R_IR, R_TIR, R_ZERO, R_ONE, R_MINUS,
       R_AMASK, R_SMASK, R_A, R_B, R_C, R_D, R_E, R_F };
typedef enum {
    OP_INVALID=-1, OP_LODD, OP_STOD, OP_ADDD, OP_SUBD, OP_JPOS, OP_JZER,
    OP_JUMP, OP_LOCO, OP_LODL, OP_STOL, OP_ADDL, OP_SUBL, OP_JNEG,
    OP_JNZE, OP_CALL, OP_PSHI, OP_POPI, OP_PUSH, OP_POP, OP_RETN,
    OP_SWAP, OP_INSP, OP_DESP, OP_COUNT
} Mic1Opcode;
typedef struct {
    Mic1Opcode op; const char *mnemonic, *meaning; char operandKind;
} Mic1InstrInfo;
extern const Mic1InstrInfo MIC1_ISA_TABLE[OP_COUNT];
extern const char *const MIC1_REGISTER_NAMES[16];
extern const uint32_t MIC1_ROM[MIC1_ROM_SIZE];
extern const char *const MIC1_ROM_TEXT[MIC1_ROM_SIZE];
extern const int MIC1_ROM_USED;

typedef struct {
    char label[160];
    unsigned mpc, subcycle;
    int pc, ac, sp, mar, mbr, ir;
    int writeAddress; /* -1 quando nao houve escrita; inclui escritas de zero */
} MicroTraceEntry;

/* Estado integral: os latches e os pedidos de memoria sobrevivem a cada passo. */
typedef struct {
    uint16_t r[16], memory[MIC1_MEM_SIZE];
    uint16_t mar, mbr, mbrRead, mbrWrite, latchA, latchB, aluA, aluOut, shiftOut;
    uint32_t mir;
    unsigned mpc, currentMicroAddress, nextSubcycle;
    bool negative, zero, halted, readReady;
    unsigned memoryPending; /* 0=ociosa, 1=leitura, 2=escrita */
    uint16_t pendingAddress, pendingValue, readValue;
    uint32_t cyclesExecuted, subcyclesExecuted, instructionsExecuted;
    int instructionAddress;
    char errorMessage[160];
    MicroTraceEntry trace[MIC1_MAX_TRACE];
    unsigned traceCount;
    Mic1Cache cache;
} Mic1State;

static inline int mic1_signed(uint16_t x) { return x < 32768 ? x : (int)x-65536; }
void mic1_init(Mic1State *cpu, const int16_t *program, size_t length);
void mic1_reset(Mic1State *cpu);
void mic1_step_subcycle(Mic1State *cpu);
void mic1_step_micro(Mic1State *cpu);
void mic1_step_macro(Mic1State *cpu);
bool mic1_at_boundary(const Mic1State *cpu);
void mic1_decode(uint16_t word, Mic1Opcode *op, int *x, int *y);
int mic1_encode(const char *mnemonic, long operand, uint16_t *word, char *err, size_t size);
const char *mic1_mnemonic_name(Mic1Opcode op);
#endif
