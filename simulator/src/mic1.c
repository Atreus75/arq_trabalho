/* MIC-1 de 16 bits: um unico caminho de dados, controlado por palavras da ROM.
 * O opcode MAC-1 nunca executa a operacao diretamente no motor.
 * Nivel de simulacao: transferencias entre registradores e quatro subciclos.
 */
#include "mic1.h"
#include <stdio.h>
#include <string.h>

const char *const MIC1_REGISTER_NAMES[16] = {
    "PC","AC","SP","IR","TIR","0","1","-1","AMASK","SMASK","A","B","C","D","E","F"
};
static unsigned field(uint32_t word, unsigned shift, unsigned mask) {
    return (word >> shift) & mask;
}
static void fail(Mic1State *cpu, const char *message) {
    cpu->halted = true;
    snprintf(cpu->errorMessage, sizeof(cpu->errorMessage), "%s", message);
}
bool mic1_at_boundary(const Mic1State *cpu) {
    return cpu->mpc == 0 && cpu->nextSubcycle == 1;
}
void mic1_init(Mic1State *cpu, const int16_t *program, size_t length) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->r[R_SP]=4096; cpu->r[R_ONE]=1; cpu->r[R_MINUS]=0xffff;
    cpu->r[R_AMASK]=0x0fff; cpu->r[R_SMASK]=0x00ff;
    cpu->nextSubcycle=1;
    cpu->instructionAddress=0;
    cache_init_default(&cpu->cache);
    if (length > MIC1_MEM_SIZE) { fail(cpu,"Programa excede 4096 palavras."); return; }
    for (size_t i=0;i<length;i++) cpu->memory[i]=(uint16_t)program[i];
}
void mic1_reset(Mic1State *cpu) {
    cpu->r[R_PC]=0; cpu->r[R_AC]=0; cpu->r[R_SP]=4096; cpu->r[R_IR]=0; cpu->r[R_TIR]=0;
    cpu->r[R_ZERO]=0; cpu->r[R_ONE]=1; cpu->r[R_MINUS]=0xffff;
    cpu->r[R_AMASK]=0x0fff; cpu->r[R_SMASK]=0x00ff;
    for (int i=R_A; i<=R_F; i++) cpu->r[i]=0;
    cpu->mar=0; cpu->mbr=0; cpu->mbrRead=0; cpu->mbrWrite=0;
    cpu->latchA=0; cpu->latchB=0; cpu->aluA=0; cpu->aluOut=0; cpu->shiftOut=0;
    cpu->mir=0; cpu->mpc=0; cpu->currentMicroAddress=0; cpu->nextSubcycle=1;
    cpu->negative=false; cpu->zero=false; cpu->halted=false; cpu->readReady=false;
    cpu->memoryPending=0; cpu->pendingAddress=0; cpu->pendingValue=0; cpu->readValue=0;
    cpu->cyclesExecuted=0; cpu->subcyclesExecuted=0; cpu->instructionsExecuted=0;
    cpu->instructionAddress=0;
    cpu->errorMessage[0]='\0';
    cpu->traceCount=0;
    cache_reset(&cpu->cache);
}
static void record(Mic1State *c, unsigned sub, const char *event, int writeAddress) {
    if (c->traceCount>=MIC1_MAX_TRACE) return;
    MicroTraceEntry *t=&c->trace[c->traceCount++];
    t->mpc=c->currentMicroAddress; t->subcycle=sub;
    t->pc=c->r[R_PC]; t->ac=mic1_signed(c->r[R_AC]); t->sp=mic1_signed(c->r[R_SP]);
    t->mar=c->mar; t->mbr=mic1_signed(c->mbr); t->ir=c->r[R_IR];
    t->writeAddress=writeAddress;
    snprintf(t->label,sizeof(t->label),"%s",event);
}
static void subcycle(Mic1State *c) {
    if (c->halted) return;
    unsigned sub=c->nextSubcycle;
    char event[160]; int writeAddress=-1;
    if (sub==1) {
        if (c->mpc>=(unsigned)MIC1_ROM_USED) { fail(c,"MPC fora do microprograma."); return; }
        if (c->mpc==0) c->instructionAddress=c->r[R_PC]&MIC1_ADDR_MASK;
        c->currentMicroAddress=c->mpc;
        c->mir=MIC1_ROM[c->mpc];
        snprintf(event,sizeof(event),"S1: MIR <- ROM[%u]",c->mpc);
    } else if (sub==2) {
        c->latchA=c->r[field(c->mir,8,15)];
        c->latchB=c->r[field(c->mir,12,15)];
        c->aluA=field(c->mir,31,1) ? c->mbrRead : c->latchA;
        snprintf(event,sizeof(event),"S2: A <- %s; B <- %s; AMUX = %s",
            MIC1_REGISTER_NAMES[field(c->mir,8,15)],MIC1_REGISTER_NAMES[field(c->mir,12,15)],
            field(c->mir,31,1)?"MBR":"A");
    } else if (sub==3) {
        switch (field(c->mir,27,3)) {
            case 0: c->aluOut=(uint16_t)((uint32_t)c->aluA+c->latchB); break;
            case 1: c->aluOut=c->aluA & c->latchB; break;
            case 2: c->aluOut=c->aluA; break;
            case 3: c->aluOut=(uint16_t)~c->aluA; break;
        }
        /* N e Z pertencem a saida da ULA ANTES do deslocador. */
        c->negative=(c->aluOut & 0x8000)!=0; c->zero=c->aluOut==0;
        switch (field(c->mir,25,3)) {
            case 0: c->shiftOut=c->aluOut; break;
            case 1: c->shiftOut=(c->aluOut >> 1) | (c->aluOut & 0x8000); break; /* aritmetico a direita */
            case 2: c->shiftOut=(uint16_t)((uint32_t)c->aluOut << 1); break;
            default: fail(c,"Controle do deslocador invalido."); return;
        }
        snprintf(event,sizeof(event),"S3: ULA = %d; deslocador = %d; N=%d Z=%d",
            mic1_signed(c->aluOut),mic1_signed(c->shiftOut),c->negative,c->zero);
    } else {
        unsigned rd=field(c->mir,22,1), wr=field(c->mir,21,1);
        if (field(c->mir,23,1)) c->mar=c->latchB & MIC1_ADDR_MASK;
        if (field(c->mir,24,1)) { c->mbrWrite=c->shiftOut; c->mbr=c->shiftOut; }
        if (field(c->mir,20,1)) {
            unsigned dst=field(c->mir,16,15);
            if (dst<R_ZERO || dst>R_SMASK) {
                c->r[dst]=c->shiftOut;
            }
        }
        unsigned request=rd?1u:wr?2u:0u;
        if (c->memoryPending) {
            if (request!=c->memoryPending) {
                fail(c,"RD/WR deve permanecer ativo durante dois ciclos consecutivos.");
                return;
            }
            if (request==1) {
                bool hit = false;
                c->mbrRead=cache_read(&c->cache, c->pendingAddress, c->memory, &hit);
                c->mbr=c->mbrRead;
            } else {
                bool hit = false;
                cache_write(&c->cache, c->pendingAddress, c->pendingValue, c->memory, &hit);
                c->mbrWrite=c->pendingValue;
                writeAddress=c->pendingAddress;
            }
            c->memoryPending=0;
        } else if (request) {
            c->memoryPending=request;
            c->pendingAddress=c->mar;
            c->pendingValue=c->mbrWrite;
        }
        unsigned cond=field(c->mir,29,3);
        bool jump=cond==3 || (cond==1 && c->negative) || (cond==2 && c->zero);
        unsigned next=jump?field(c->mir,0,255):(c->mpc+1)&255;
        if (next==0 && !c->halted) c->instructionsExecuted++;
        c->mpc=next;
        snprintf(event,sizeof(event),"S4: %s%s",MIC1_ROM_TEXT[c->currentMicroAddress],
            writeAddress>=0?" [memoria gravada]":"");
        c->cyclesExecuted++;
    }
    c->subcyclesExecuted++;
    c->nextSubcycle=sub==4?1:sub+1;
    record(c,sub,event,writeAddress);
}
void mic1_step_subcycle(Mic1State *c) {
    c->traceCount=0; subcycle(c);
}
void mic1_step_micro(Mic1State *c) {
    c->traceCount=0;
    do { subcycle(c); } while (!c->halted && c->nextSubcycle!=1);
}
void mic1_step_macro(Mic1State *c) {
    c->traceCount=0;
    for (unsigned i=0;i<512 && !c->halted;i++) {
        subcycle(c);
        if (mic1_at_boundary(c)) return;
    }
    if (!c->halted) fail(c,"Limite de subciclos por instrucao excedido.");
}
