// Simulador do MIC-1: caminho de dados e 4 subciclos
#include "mic1.h"
#include <stdio.h>
#include <string.h>

const char *const MIC1_REGISTER_NAMES[16] = {
    "PC","AC","SP","IR","TIR","0","1","-1","AMASK","SMASK","A","B","C","D","E","F"
};
static unsigned field(uint32_t palavra, unsigned desloc, unsigned mascara) {
    return (palavra >> desloc) & mascara;
}
static void fail(Mic1State *cpu, const char *msg) {
    cpu->parado = true;
    snprintf(cpu->msgErro, sizeof(cpu->msgErro), "%s", msg);
}
bool mic1_at_boundary(const Mic1State *cpu) {
    return cpu->mpc == 0 && cpu->proxSubciclo == 1;
}
void mic1_init(Mic1State *cpu, const int16_t *programa, size_t length) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->r[R_SP]=4096; cpu->r[R_ONE]=1; cpu->r[R_MINUS]=0xffff;
    cpu->r[R_AMASK]=0x0fff; cpu->r[R_SMASK]=0x00ff;
    cpu->proxSubciclo=1;
    cpu->endInstrucao=0;
    cache_init_default(&cpu->cache);
    if (length > MIC1_MEM_SIZE) { fail(cpu,"Programa excede 4096 palavras."); return; }
    for (size_t i=0;i<length;i++) cpu->memoria[i]=(uint16_t)programa[i];
}
void mic1_reset(Mic1State *cpu) {
    cpu->r[R_PC]=0; cpu->r[R_AC]=0; cpu->r[R_SP]=4096; cpu->r[R_IR]=0; cpu->r[R_TIR]=0;
    cpu->r[R_ZERO]=0; cpu->r[R_ONE]=1; cpu->r[R_MINUS]=0xffff;
    cpu->r[R_AMASK]=0x0fff; cpu->r[R_SMASK]=0x00ff;
    for (int i=R_A; i<=R_F; i++) cpu->r[i]=0;
    cpu->mar=0; cpu->mbr=0; cpu->mbrLeitura=0; cpu->mbrEscrita=0;
    cpu->latchA=0; cpu->latchB=0; cpu->ulaA=0; cpu->saidaUla=0; cpu->saidaDeslocador=0;
    cpu->mir=0; cpu->mpc=0; cpu->endMicroAtual=0; cpu->proxSubciclo=1;
    cpu->negativo=false; cpu->zero=false; cpu->parado=false; cpu->leituraPronta=false;
    cpu->memPendente=0; cpu->endPendente=0; cpu->valorPendente=0; cpu->valorLido=0;
    cpu->ciclosExecutados=0; cpu->subciclosExecutados=0; cpu->instrucoesExecutadas=0;
    cpu->endInstrucao=0;
    cpu->msgErro[0]='\0';
    cpu->qtdHistorico=0;
    cache_reset(&cpu->cache);
}
static void record(Mic1State *c, unsigned sub, const char *evento, int endEscrita) {
    if (c->qtdHistorico>=MIC1_MAX_TRACE) return;
    MicroTraceEntry *t=&c->historico[c->qtdHistorico++];
    t->mpc=c->endMicroAtual; t->subciclo=sub;
    t->pc=c->r[R_PC]; t->ac=mic1_signed(c->r[R_AC]); t->sp=mic1_signed(c->r[R_SP]);
    t->mar=c->mar; t->mbr=mic1_signed(c->mbr); t->ir=c->r[R_IR];
    t->endEscrita=endEscrita;
    snprintf(t->texto,sizeof(t->texto),"%s",evento);
}
static void subcycle(Mic1State *c) {
    if (c->parado) return;
    unsigned sub=c->proxSubciclo;
    char evento[160]; int endEscrita=-1;
    if (sub==1) {
        if (c->mpc>=(unsigned)MIC1_ROM_USED) { fail(c,"MPC fora do microprograma."); return; }
        if (c->mpc==0) c->endInstrucao=c->r[R_PC]&MIC1_ADDR_MASK;
        c->endMicroAtual=c->mpc;
        c->mir=MIC1_ROM[c->mpc];
        snprintf(evento,sizeof(evento),"S1: MIR <- ROM[%u]",c->mpc);
    } else if (sub==2) {
        c->latchA=c->r[field(c->mir,8,15)];
        c->latchB=c->r[field(c->mir,12,15)];
        c->ulaA=field(c->mir,31,1) ? c->mbrLeitura : c->latchA;
        snprintf(evento,sizeof(evento),"S2: A <- %s; B <- %s; AMUX = %s",
            MIC1_REGISTER_NAMES[field(c->mir,8,15)],MIC1_REGISTER_NAMES[field(c->mir,12,15)],
            field(c->mir,31,1)?"MBR":"A");
    } else if (sub==3) {
        switch (field(c->mir,27,3)) {
            case 0: c->saidaUla=(uint16_t)((uint32_t)c->ulaA+c->latchB); break;
            case 1: c->saidaUla=c->ulaA & c->latchB; break;
            case 2: c->saidaUla=c->ulaA; break;
            case 3: c->saidaUla=(uint16_t)~c->ulaA; break;
        }
        /* N e Z pertencem a saida da ULA ANTES do deslocador. */
        c->negativo=(c->saidaUla & 0x8000)!=0; c->zero=c->saidaUla==0;
        switch (field(c->mir,25,3)) {
            case 0: c->saidaDeslocador=c->saidaUla; break;
            case 1: c->saidaDeslocador=(c->saidaUla >> 1) | (c->saidaUla & 0x8000); break; /* aritmetico a direita */
            case 2: c->saidaDeslocador=(uint16_t)((uint32_t)c->saidaUla << 1); break;
            default: fail(c,"Controle do deslocador invalido."); return;
        }
        snprintf(evento,sizeof(evento),"S3: ULA = %d; deslocador = %d; N=%d Z=%d",
            mic1_signed(c->saidaUla),mic1_signed(c->saidaDeslocador),c->negativo,c->zero);
    } else {
        unsigned rd=field(c->mir,22,1), wr=field(c->mir,21,1);
        if (field(c->mir,23,1)) c->mar=c->latchB & MIC1_ADDR_MASK;
        if (field(c->mir,24,1)) { c->mbrEscrita=c->saidaDeslocador; c->mbr=c->saidaDeslocador; }
        if (field(c->mir,20,1)) {
            unsigned dest=field(c->mir,16,15);
            if (dest<R_ZERO || dest>R_SMASK) {
                c->r[dest]=c->saidaDeslocador;
            }
        }
        unsigned pedido=rd?1u:wr?2u:0u;
        if (c->memPendente) {
            if (pedido!=c->memPendente) {
                fail(c,"RD/WR deve permanecer ativo durante dois ciclos consecutivos.");
                return;
            }
            if (pedido==1) {
                bool hit = false;
                c->mbrLeitura=cache_read(&c->cache, c->endPendente, c->memoria, &hit);
                c->mbr=c->mbrLeitura;
            } else {
                bool hit = false;
                cache_write(&c->cache, c->endPendente, c->valorPendente, c->memoria, &hit);
                c->mbrEscrita=c->valorPendente;
                endEscrita=c->endPendente;
            }
            c->memPendente=0;
        } else if (pedido) {
            c->memPendente=pedido;
            c->endPendente=c->mar;
            c->valorPendente=c->mbrEscrita;
        }
        unsigned cond=field(c->mir,29,3);
        bool salto=cond==3 || (cond==1 && c->negativo) || (cond==2 && c->zero);
        unsigned prox=salto?field(c->mir,0,255):(c->mpc+1)&255;
        if (prox==0 && !c->parado) c->instrucoesExecutadas++;
        c->mpc=prox;
        snprintf(evento,sizeof(evento),"S4: %s%s",MIC1_ROM_TEXT[c->endMicroAtual],
            endEscrita>=0?" [memoria gravada]":"");
        c->ciclosExecutados++;
    }
    c->subciclosExecutados++;
    c->proxSubciclo=sub==4?1:sub+1;
    record(c,sub,evento,endEscrita);
}
void mic1_step_subcycle(Mic1State *c) {
    c->qtdHistorico=0; subcycle(c);
}
void mic1_step_micro(Mic1State *c) {
    c->qtdHistorico=0;
    do { subcycle(c); } while (!c->parado && c->proxSubciclo!=1);
}
void mic1_step_macro(Mic1State *c) {
    c->qtdHistorico=0;
    for (unsigned i=0;i<512 && !c->parado;i++) {
        subcycle(c);
        if (mic1_at_boundary(c)) return;
    }
    if (!c->parado) fail(c,"Limite de subciclos por instrucao excedido.");
}
