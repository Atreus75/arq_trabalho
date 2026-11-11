/* Microprograma MAC-1 classico, identico a MemoriaDeControle.java.
 * Referencia didatica: Tanenbaum e csbassin/SimuladorVinod (9547d11).
 * Campos: AMUX COND ALU SH MBR MAR RD WR ENC C B A ADDR.
 */
#include "mic1.h"
#define M(amux,cond,alu,sh,mbr,mar,rd,wr,enc,c,b,a,addr) \
    (((uint32_t)(amux)<<31)|((uint32_t)(cond)<<29)|((uint32_t)(alu)<<27)| \
     ((uint32_t)(sh)<<25)|((uint32_t)(mbr)<<24)|((uint32_t)(mar)<<23)| \
     ((uint32_t)(rd)<<22)|((uint32_t)(wr)<<21)|((uint32_t)(enc)<<20)| \
     ((uint32_t)(c)<<16)|((uint32_t)(b)<<12)|((uint32_t)(a)<<8)|(addr))
const int MIC1_ROM_USED = 79;
const uint32_t MIC1_ROM[MIC1_ROM_SIZE] = {
    /* 00: mar:=pc; rd; */
    M(0,0,2,0,0,1,1,0,0,0,0,0,0),
    /* 01: pc:=pc + 1; rd; */
    M(0,0,0,0,0,0,1,0,1,0,6,0,0),
    /* 02: ir:=mbr; if n then goto 28; */
    M(1,1,2,0,0,0,0,0,1,3,0,0,28),
    /* 03: tir:=lshift(ir + ir); if n then goto 19; */
    M(0,1,0,2,0,0,0,0,1,4,3,3,19),
    /* 04: tir:=lshift(tir); if n then goto 11; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,11),
    /* 05: alu:=tir; if n then goto 9; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,9),
    /* 06: mar:=ir; rd; */
    M(0,0,2,0,0,1,1,0,0,0,3,0,0),
    /* 07: rd; */
    M(0,0,0,0,0,0,1,0,0,0,0,0,0),
    /* 08: ac:=mbr; goto 0; */
    M(1,3,2,0,0,0,0,0,1,1,0,0,0),
    /* 09: mar:=ir; mbr:=ac; wr; */
    M(0,0,2,0,1,1,0,1,0,0,3,1,0),
    /* 10: wr; goto 0; */
    M(0,3,0,0,0,0,0,1,0,0,0,0,0),
    /* 11: alu:=tir; if n then goto 15; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,15),
    /* 12: mar:=ir; rd; */
    M(0,0,0,0,0,1,1,0,0,0,3,0,0),
    /* 13: rd; */
    M(0,0,0,0,0,0,1,0,0,0,0,0,0),
    /* 14: ac:=mbr + ac; goto 0; */
    M(1,3,0,0,0,0,0,0,1,1,1,0,0),
    /* 15: mar:=ir; rd; */
    M(0,0,2,0,0,1,1,0,0,0,3,0,0),
    /* 16: ac:=ac + 1; rd; */
    M(0,0,0,0,0,0,1,0,1,1,6,1,0),
    /* 17: a:=inv(mbr); */
    M(1,0,3,0,0,0,0,0,1,10,0,0,0),
    /* 18: ac:=ac + a; goto 0; */
    M(0,3,0,0,0,0,0,0,1,1,10,1,0),
    /* 19: tir:=lshift(tir); if n then goto 25; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,25),
    /* 20: alu:=tir; if n then goto 23; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,23),
    /* 21: alu:=ac; if n then goto 0; */
    M(0,1,2,0,0,0,0,0,0,0,0,1,0),
    /* 22: pc:=band(ir,amask); goto 0; */
    M(0,3,1,0,0,0,0,0,1,0,3,8,0),
    /* 23: alu:=ac; if z then goto 22; */
    M(0,2,2,0,0,0,0,0,0,0,0,1,22),
    /* 24: goto 0; */
    M(0,3,0,0,0,0,0,0,0,0,0,0,0),
    /* 25: alu:=tir; if n then goto 27; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,27),
    /* 26: pc:=band(ir,amask); goto 0; */
    M(0,3,1,0,0,0,0,0,1,0,3,8,0),
    /* 27: ac:=band(ir,amask); goto 0; */
    M(0,3,1,0,0,0,0,0,1,1,3,8,0),
    /* 28: tir:=lshift(ir + ir); if n then goto 40; */
    M(0,1,0,2,0,0,0,0,1,4,3,3,40),
    /* 29: tir:=lshift(tir); if n then goto 35; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,35),
    /* 30: alu:=tir; if n then goto 33; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,33),
    /* 31: a:=ir + sp; */
    M(0,0,0,0,0,0,0,0,1,10,3,2,0),
    /* 32: mar:=a; rd; goto 7; */
    M(0,3,0,0,0,1,1,0,0,0,10,0,7),
    /* 33: a:=ir + sp; */
    M(0,0,0,0,0,0,0,0,1,10,3,2,0),
    /* 34: mar:=a; mbr:=ac; wr; goto 10; */
    M(0,3,2,0,1,1,0,1,0,0,10,1,10),
    /* 35: alu:=tir; if n then goto 38; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,38),
    /* 36: a:=ir + sp; */
    M(0,0,0,0,0,0,0,0,1,10,3,2,0),
    /* 37: mar:=a; rd; goto 13; */
    M(0,3,0,0,0,1,1,0,0,0,10,0,13),
    /* 38: a:=ir + sp; */
    M(0,0,0,0,0,0,0,0,1,10,3,2,0),
    /* 39: mar:=a; rd; goto 16 ; */
    M(0,3,0,0,0,1,1,0,0,0,10,0,16),
    /* 40: tir:=lshift(tir); if n then goto 46; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,46),
    /* 41: alu:=tir; if n then goto 44; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,44),
    /* 42: alu:=ac; if n then goto 22; */
    M(0,1,2,0,0,0,0,0,0,0,0,1,22),
    /* 43: goto 0; */
    M(0,3,0,0,0,0,0,0,0,0,0,0,0),
    /* 44: alu:=ac; if z then goto 0; */
    M(0,2,2,0,0,0,0,0,0,0,0,1,0),
    /* 45: pc:=band(ir,amask); goto 0; */
    M(0,3,1,0,0,0,0,0,1,0,3,8,0),
    /* 46: tir:=lshift(tir); if n then goto 50; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,50),
    /* 47: sp:=sp + (-1); */
    M(0,0,0,0,0,0,0,0,1,2,2,7,0),
    /* 48: mar:=sp; mbr:=pc; wr; */
    M(0,0,2,0,1,1,0,1,0,0,2,0,0),
    /* 49: pc:=band(ir,amask); wr; goto 0; */
    M(0,3,1,0,0,0,0,1,1,0,3,8,0),
    /* 50: tir:=lshift(tir); if n then goto 65; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,65),
    /* 51: tir:=lshift(tir); if n then goto 59; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,59),
    /* 52: alu:=tir; if n then goto 56; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,56),
    /* 53: mar:=ac; rd; */
    M(0,0,0,0,0,1,1,0,0,0,1,0,0),
    /* 54: sp:=sp + (-1); rd; */
    M(0,0,0,0,0,0,1,0,1,2,2,7,0),
    /* 55: mar:=sp; wr; goto 10; */
    M(0,3,0,0,0,1,0,1,0,0,2,0,10),
    /* 56: mar:=sp; sp:=sp + 1; rd; */
    M(0,0,0,0,0,1,1,0,1,2,2,6,0),
    /* 57: rd; */
    M(0,0,0,0,0,0,1,0,0,0,0,0,0),
    /* 58: mar:=ac; wr; goto 10; */
    M(0,3,0,0,0,1,0,1,0,0,1,0,10),
    /* 59: alu:=tir; if n then goto 62; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,62),
    /* 60: sp:=sp + (-1); */
    M(0,0,0,0,0,0,0,0,1,2,2,7,0),
    /* 61: mar:=sp; mbr:=ac; wr; goto 10; */
    M(0,3,2,0,1,1,0,1,0,0,2,1,10),
    /* 62: mar:=sp; sp:=sp + 1; rd; */
    M(0,0,0,0,0,1,1,0,1,2,2,6,0),
    /* 63: rd; */
    M(0,0,0,0,0,0,1,0,0,0,0,0,0),
    /* 64: ac:=mbr; goto 0; */
    M(1,3,2,0,0,0,0,0,1,1,0,0,0),
    /* 65: tir:=lshift(tir); if n then goto 73; */
    M(0,1,2,2,0,0,0,0,1,4,0,4,73),
    /* 66: alu:=tir; if n then goto 70; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,70),
    /* 67: mar:=sp; sp:=sp + 1; rd; */
    M(0,0,0,0,0,1,1,0,1,2,2,6,0),
    /* 68: rd; */
    M(0,0,0,0,0,0,1,0,0,0,0,0,0),
    /* 69: pc:=mbr; goto 0; */
    M(1,3,2,0,0,0,0,0,1,0,0,0,0),
    /* 70: a:=ac; */
    M(0,0,2,0,0,0,0,0,1,10,0,1,0),
    /* 71: ac:=sp; */
    M(0,0,2,0,0,0,0,0,1,1,0,2,0),
    /* 72: sp:=a; goto 0; */
    M(0,3,2,0,0,0,0,0,1,2,0,10,0),
    /* 73: alu:=tir; if n then goto 76; */
    M(0,1,2,0,0,0,0,0,0,0,0,4,76),
    /* 74: a:=band(ir,smask); */
    M(0,0,1,0,0,0,0,0,1,10,3,9,0),
    /* 75: sp:=sp + a; goto 0; */
    M(0,3,0,0,0,0,0,0,1,2,2,10,0),
    /* 76: a:=band(ir, smask); */
    M(0,0,1,0,0,0,0,0,1,10,3,9,0),
    /* 77: a:=inv(a); */
    M(0,0,3,0,0,0,0,0,1,10,0,10,0),
    /* 78: a:=a + 1; goto 75; */
    M(0,3,0,0,0,0,0,0,1,10,10,6,75),
};
const char *const MIC1_ROM_TEXT[MIC1_ROM_SIZE] = {
    "mar:=pc; rd;",
    "pc:=pc + 1; rd;",
    "ir:=mbr; if n then goto 28;",
    "tir:=lshift(ir + ir); if n then goto 19;",
    "tir:=lshift(tir); if n then goto 11;",
    "alu:=tir; if n then goto 9;",
    "mar:=ir; rd;",
    "rd;",
    "ac:=mbr; goto 0;",
    "mar:=ir; mbr:=ac; wr;",
    "wr; goto 0;",
    "alu:=tir; if n then goto 15;",
    "mar:=ir; rd;",
    "rd;",
    "ac:=mbr + ac; goto 0;",
    "mar:=ir; rd;",
    "ac:=ac + 1; rd;",
    "a:=inv(mbr);",
    "ac:=ac + a; goto 0;",
    "tir:=lshift(tir); if n then goto 25;",
    "alu:=tir; if n then goto 23;",
    "alu:=ac; if n then goto 0;",
    "pc:=band(ir,amask); goto 0;",
    "alu:=ac; if z then goto 22;",
    "goto 0;",
    "alu:=tir; if n then goto 27;",
    "pc:=band(ir,amask); goto 0;",
    "ac:=band(ir,amask); goto 0;",
    "tir:=lshift(ir + ir); if n then goto 40;",
    "tir:=lshift(tir); if n then goto 35;",
    "alu:=tir; if n then goto 33;",
    "a:=ir + sp;",
    "mar:=a; rd; goto 7;",
    "a:=ir + sp;",
    "mar:=a; mbr:=ac; wr; goto 10;",
    "alu:=tir; if n then goto 38;",
    "a:=ir + sp;",
    "mar:=a; rd; goto 13;",
    "a:=ir + sp;",
    "mar:=a; rd; goto 16 ;",
    "tir:=lshift(tir); if n then goto 46;",
    "alu:=tir; if n then goto 44;",
    "alu:=ac; if n then goto 22;",
    "goto 0;",
    "alu:=ac; if z then goto 0;",
    "pc:=band(ir,amask); goto 0;",
    "tir:=lshift(tir); if n then goto 50;",
    "sp:=sp + (-1);",
    "mar:=sp; mbr:=pc; wr;",
    "pc:=band(ir,amask); wr; goto 0;",
    "tir:=lshift(tir); if n then goto 65;",
    "tir:=lshift(tir); if n then goto 59;",
    "alu:=tir; if n then goto 56;",
    "mar:=ac; rd;",
    "sp:=sp + (-1); rd;",
    "mar:=sp; wr; goto 10;",
    "mar:=sp; sp:=sp + 1; rd;",
    "rd;",
    "mar:=ac; wr; goto 10;",
    "alu:=tir; if n then goto 62;",
    "sp:=sp + (-1);",
    "mar:=sp; mbr:=ac; wr; goto 10;",
    "mar:=sp; sp:=sp + 1; rd;",
    "rd;",
    "ac:=mbr; goto 0;",
    "tir:=lshift(tir); if n then goto 73;",
    "alu:=tir; if n then goto 70;",
    "mar:=sp; sp:=sp + 1; rd;",
    "rd;",
    "pc:=mbr; goto 0;",
    "a:=ac;",
    "ac:=sp;",
    "sp:=a; goto 0;",
    "alu:=tir; if n then goto 76;",
    "a:=band(ir,smask);",
    "sp:=sp + a; goto 0;",
    "a:=band(ir, smask);",
    "a:=inv(a);",
    "a:=a + 1; goto 75;",
};
