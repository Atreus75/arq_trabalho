/**
 * Tipos do dominio MIC-1 no front-end.
 * Contrato JSON do motor C -- qualquer mudanca no formato
 * de estado da CPU ou do resultado de montagem precisa ser replicada
 * tambem na serializacao de simulator/src/protocol.c.
 */

export type CpuPhase = "FETCH" | "EXECUTE";

export interface MicroTraceEntry {
  label: string; mpc: number; subcycle: number; pc: number; ac: number;
  sp: number; mar: number; mbr: number; ir: number; writeAddress: number;
}
export interface Mic1CpuState {
  pc: number; sp: number; ac: number; ir: number; mar: number; mbr: number;
  mbrRead?: number; mbrWrite?: number;
  registers: number[]; memory: number[];
  mir: number; mpc: number; currentMicroAddress: number; nextSubcycle: number;
  latchA: number; latchB: number; aluA: number; aluOut: number; shiftOut: number;
  negative: boolean; zero: boolean; readReady: boolean;
  memoryPending: number; pendingAddress: number; pendingValue: number; readValue: number;
  halted: boolean; errorMessage: string; microInstruction: string;
  atInstructionBoundary: boolean; instructionAddress: number;
  instructionsExecuted: number; cyclesExecuted: number; subcyclesExecuted: number;
  trace: MicroTraceEntry[];
  cache?: Mic1CacheState;
}
export type StepUnit = "instruction" | "cycle" | "subcycle";

export interface AssembledWord {
  address: number;
  word: number;
  binaryString: string;
  mnemonic: string;
  operandDisplay: string;
  sourceLine: number;
  sourceText: string;
}

export interface AssembleError {
  lineNumber: number;
  message: string;
  sourceText: string;
}

export interface AssembleResult {
  success: boolean;
  program: AssembledWord[];
  errors: AssembleError[];
  labelTable: Record<string, number>;
  savedAs?: string;
}

export const MIC1_MEM_SIZE = 4096;

export type CacheMapping = "direct" | "fully" | "set";
export type CacheReplacement = "lru" | "fifo";
export type CacheWritePolicy = "write-through" | "write-back";

export interface Mic1CacheConfig {
  enabled: boolean;
  mapping: CacheMapping;
  replacement: CacheReplacement;
  writePolicy: CacheWritePolicy;
  capacityWords: number;
  blockSizeWords: number;
  numWays: number;
  penaltyCycles: number;
}

export interface Mic1CacheStats {
  readHits: number;
  readMisses: number;
  writeHits: number;
  writeMisses: number;
  totalHits: number;
  totalMisses: number;
  hitRate: number;
  missRate: number;
  penaltyCyclesTotal: number;
}

export interface Mic1CacheLastAccess {
  hasAccess: boolean;
  event: string;
  address: number;
  block: number;
  setIndex: number;
  wayIndex: number;
  tag: number;
  hit: boolean;
}

export interface Mic1CacheLine {
  index: number;
  set: number;
  way: number;
  valid: boolean;
  dirty: boolean;
  tag: number;
  blockAddress: number;
  words: number[];
  lastAccessTime: number;
  loadTime: number;
}

export interface Mic1CacheState {
  enabled: boolean;
  mapping: CacheMapping;
  replacement: CacheReplacement;
  writePolicy: CacheWritePolicy;
  capacityWords: number;
  blockSizeWords: number;
  numSets: number;
  numWays: number;
  penaltyCycles: number;
  stats: Mic1CacheStats;
  lastAccess: Mic1CacheLastAccess;
  lines: Mic1CacheLine[];
}

/** Modo de execucao selecionado na UI. */
export type RunMode = "idle" | "running-macro" | "running-micro";

/** Normaliza uma palavra de 16 bits (que pode chegar negativa, pois o
 * C usa int16_t) para sua representacao SEM SINAL de 0..65535, usada
 * para exibir binario e hexadecimal. O valor COM sinal (para exibir
 * "AC = -3", por exemplo) continua vindo direto do backend. */
export function toUnsigned16(value: number): number {
  return value & 0xffff;
}

export function toBinaryString(value: number): string {
  return toUnsigned16(value).toString(2).padStart(16, "0");
}

export function toHexString(value: number): string {
  return toUnsigned16(value).toString(16).padStart(4, "0").toUpperCase();
}

/** Apenas exibicao: a execucao continua no motor C. */
export function formatInstruction(word: number): string {
  const value = word & 0xffff;
  if ((value & 0xfc00) === 0xe000) {
    return `CALL ${value & 0x3ff}`;
  }
  const names = [
    "LODD", "STOD", "ADDD", "SUBD",
    "JPOS", "JZER", "JUMP", "LOCO",
    "LODL", "STOL", "ADDL", "SUBL",
    "JNEG", "JNZE"
  ];
  const opcode = value >>> 12;
  if (opcode < 14) return `${names[opcode]} ${value & 0xfff}`;
  if ((value & 0xff00) === 0xfc00) return `INSP ${value & 0xff}`;
  if ((value & 0xff00) === 0xfe00) return `DESP ${value & 0xff}`;
  const extended: Record<number, string> = {
    0xf000: "PSHI",
    0xf200: "POPI",
    0xf400: "PUSH",
    0xf600: "POP",
    0xf800: "RETN",
    0xfa00: "SWAP"
  };
  return extended[value] ?? `Inválida (0x${toHexString(value)})`;
}
