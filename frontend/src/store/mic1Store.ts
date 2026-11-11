/** Estado da interface. O motor C continua sendo o unico executor. */
import { create } from "zustand";
import type { AssembleResult, Mic1CpuState, Mic1CacheState, Mic1CacheConfig, StepUnit } from "../types/mic1";
import { assembleProgram, simInit, simStepMacro, simStepMicro, simStepSubcycle } from "../api/client";
import { EXAMPLES } from "./examples";

export const CLOCK_SPEEDS = [
  { label: "Lento", intervalMs: 1000 },
  { label: "Normal", intervalMs: 350 },
  { label: "Rápido", intervalMs: 60 },
] as const;

let generation = 0;
const describeError = (error: unknown) => error instanceof Error ? error.message : String(error);

interface Mic1Store {
  sourceCode: string;
  assembleResult: AssembleResult | null;
  cpuState: Mic1CpuState | null;
  cacheState: Mic1CacheState | null;
  writtenAddresses: number[];
  isAssembling: boolean;
  isCpuInitialized: boolean;
  isStepping: boolean;
  lastError: string | null;
  runMode: "idle" | "running-macro";
  clockSpeedIndex: number;
  delayMs: number;
  executionTimeMs: number;
  runStartCount: number;
  _intervalHandle: ReturnType<typeof setInterval> | null;
  stepUnit: StepUnit;
  breakpoint: number | null;
  breakpointTriggered: boolean;
  pauseReason: string | null;

  setSourceCode: (code: string) => void;
  assemble: () => Promise<void>;
  assembleOnly: () => Promise<void>;
  loadProgramToMemory: () => Promise<void>;
  setStepUnit: (unit: StepUnit) => void;
  setBreakpoint: (address: number | null) => void;
  applyBreakpoint: (address: number | null) => void;
  setDelayMs: (ms: number) => void;
  setClockSpeedIndex: (index: number) => void;
  stepSelected: () => Promise<void>;
  _step: (unit: StepUnit) => Promise<void>;
  stepMacro: () => Promise<void>;
  startRunMacro: () => void;
  pause: () => void;
  reset: () => Promise<void>;
  configureCache: (config: Partial<Mic1CacheConfig>) => Promise<void>;
}

export const useMic1Store = create<Mic1Store>((set, get) => ({
  sourceCode: EXAMPLES[0].source,
  assembleResult: null,
  cpuState: null,
  cacheState: null,
  writtenAddresses: [],
  isAssembling: false,
  isCpuInitialized: false,
  isStepping: false,
  lastError: null,
  runMode: "idle",
  clockSpeedIndex: 0,
  delayMs: 1000,
  executionTimeMs: 0,
  runStartCount: 0,
  stepUnit: "cycle",
  breakpoint: null,
  breakpointTriggered: false,
  pauseReason: null,
  _intervalHandle: null,

  setStepUnit: (stepUnit) => { get().pause(); set({ stepUnit }); },
  setBreakpoint: (breakpoint) => set({ breakpoint }),
  applyBreakpoint: (breakpoint) => set({ breakpoint, breakpointTriggered: false, pauseReason: null }),

  setDelayMs: (delayMs: number) => {
    const clamped = Math.max(50, Math.min(5000, Math.trunc(delayMs) || 1000));
    set({ delayMs: clamped });
    if (get().runMode === "running-macro") {
      const handle = get()._intervalHandle;
      if (handle !== null) clearInterval(handle);
      let lastTime = Date.now();
      const newHandle = setInterval(() => {
        const now = Date.now();
        const delta = now - lastTime;
        lastTime = now;
        set((s) => ({ executionTimeMs: s.executionTimeMs + delta }));
        void get().stepSelected();
      }, clamped);
      set({ _intervalHandle: newHandle });
    }
  },

  setClockSpeedIndex: (index) => {
    if (!Number.isInteger(index) || index < 0 || index >= CLOCK_SPEEDS.length) return;
    const ms = CLOCK_SPEEDS[index].intervalMs;
    set({ clockSpeedIndex: index, delayMs: ms });
    if (get().runMode === "running-macro") {
      const handle = get()._intervalHandle;
      if (handle !== null) clearInterval(handle);
      let lastTime = Date.now();
      const newHandle = setInterval(() => {
        const now = Date.now();
        const delta = now - lastTime;
        lastTime = now;
        set((s) => ({ executionTimeMs: s.executionTimeMs + delta }));
        void get().stepSelected();
      }, ms);
      set({ _intervalHandle: newHandle });
    }
  },

  stepSelected: () => get()._step(get().stepUnit),
  stepMacro: () => get()._step("instruction"),

  setSourceCode: (sourceCode) => {
    generation++;
    get().pause();
    set({
      sourceCode,
      assembleResult: null,
      cpuState: null,
      cacheState: null,
      writtenAddresses: [],
      isAssembling: false,
      isStepping: false,
      isCpuInitialized: false,
      pauseReason: null,
      lastError: null,
      executionTimeMs: 0,
      breakpointTriggered: false,
    });
  },

  assembleOnly: async () => {
    get().pause();
    const id = ++generation;
    const sourceCode = get().sourceCode;
    set({ isAssembling: true, assembleResult: null, pauseReason: null, lastError: null });
    try {
      const result = await assembleProgram({ sourceCode });
      if (id !== generation) return;
      set({ assembleResult: result, isAssembling: false });
    } catch (error) {
      if (id === generation) set({ isAssembling: false, lastError: describeError(error) });
    }
  },

  loadProgramToMemory: async () => {
    const result = get().assembleResult;
    if (!result?.success || !result.program.length) {
      set({ lastError: "Monte o programa com sucesso antes de gravar na MP." });
      return;
    }
    const current = get().cpuState;
    if (!current) {
      const id = ++generation;
      set({ isAssembling: true, lastError: null, pauseReason: null });
      try {
        const cpuState = await simInit(result.program.map((word) => word.word));
        if (id === generation) {
          set({
            cpuState,
            cacheState: cpuState.cache ?? null,
            isAssembling: false,
            isCpuInitialized: true,
            writtenAddresses: result.program.map((w) => w.address),
            executionTimeMs: 0,
            breakpointTriggered: false,
          });
        }
      } catch (error) {
        if (id === generation) set({ isAssembling: false, lastError: describeError(error) });
      }
      return;
    }

    const newMemory = [...current.memory];
    const written = new Set(get().writtenAddresses);
    for (const w of result.program) {
      newMemory[w.address] = w.word;
      written.add(w.address);
    }
    const updatedState: Mic1CpuState = {
      ...current,
      memory: newMemory,
    };
    set({
      cpuState: updatedState,
      writtenAddresses: [...written].sort((a, b) => a - b),
      lastError: null,
    });
  },

  assemble: async () => {
    get().pause();
    const id = ++generation;
    const sourceCode = get().sourceCode;
    set({
      isAssembling: true,
      isStepping: false,
      cpuState: null,
      cacheState: null,
      assembleResult: null,
      isCpuInitialized: false,
      writtenAddresses: [],
      pauseReason: null,
      lastError: null,
      executionTimeMs: 0,
      breakpointTriggered: false,
    });
    try {
      const result = await assembleProgram({ sourceCode });
      if (id !== generation) return;
      set({ assembleResult: result });
      if (!result.success) { set({ isAssembling: false }); return; }
      const cpuState = await simInit(result.program.map((word) => word.word));
      if (id !== generation) return;
      set({
        cpuState,
        cacheState: cpuState.cache ?? null,
        isAssembling: false,
        isCpuInitialized: true,
        writtenAddresses: result.program.map((w) => w.address),
      });
    } catch (error) {
      if (id === generation) set({ isAssembling: false, lastError: describeError(error) });
    }
  },

  _step: async (unit) => {
    const { cpuState, isStepping, isAssembling } = get();
    if (!cpuState || cpuState.halted || isStepping || isAssembling) return;
    const id = generation;
    set({ isStepping: true, lastError: null, pauseReason: null });
    try {
      const next = await (unit === "instruction" ? simStepMacro : unit === "cycle" ? simStepMicro : simStepSubcycle)(cpuState);
      if (id !== generation) return;

      const written = new Set(get().writtenAddresses);
      for (const entry of next.trace) {
        if (entry.writeAddress >= 0) written.add(entry.writeAddress);
      }
      set({
        cpuState: next,
        cacheState: next.cache ?? get().cacheState,
        isStepping: false,
        writtenAddresses: [...written].sort((a, b) => a - b),
        lastError: next.errorMessage || null,
      });

      if (next.halted) {
        get().pause();
      } else if (get().runMode !== "idle" && get().breakpoint !== null && !get().breakpointTriggered) {
        if ((next.pc & 4095) === get().breakpoint) {
          get().pause();
          set({
            breakpointTriggered: true,
            pauseReason: `Pausa no endereço ${next.pc} (Breakpoint atingido). Clique em Executar para continuar.`,
          });
        }
      }
    } catch (error) {
      if (id !== generation) return;
      set({ isStepping: false, lastError: describeError(error) });
      get().pause();
    }
  },

  startRunMacro: () => {
    const state = get();
    if (state.runMode !== "idle" || !state.cpuState || state.cpuState.halted || state.isAssembling || state.isStepping) return;
    let lastTime = Date.now();
    const handle = setInterval(() => {
      const now = Date.now();
      const delta = now - lastTime;
      lastTime = now;
      set((s) => ({ executionTimeMs: s.executionTimeMs + delta }));
      void get().stepSelected();
    }, state.delayMs);
    set({
      runMode: "running-macro",
      _intervalHandle: handle,
      runStartCount: state.cpuState.instructionsExecuted,
    });
    void get().stepSelected();
  },

  pause: () => {
    const handle = get()._intervalHandle;
    if (handle !== null) clearInterval(handle);
    set({ runMode: "idle", _intervalHandle: null });
  },

  reset: async () => {
    get().pause();
    const result = get().assembleResult;
    if (!result?.success) return;
    const id = ++generation;
    set({
      isAssembling: true,
      isStepping: false,
      pauseReason: null,
      lastError: null,
      cpuState: null,
      cacheState: null,
      isCpuInitialized: false,
      writtenAddresses: [],
      executionTimeMs: 0,
      breakpointTriggered: false,
    });
    try {
      const cpuState = await simInit(result.program.map((word) => word.word));
      if (id === generation) {
        set({
          cpuState,
          cacheState: cpuState.cache ?? null,
          isAssembling: false,
          isCpuInitialized: true,
          writtenAddresses: result.program.map((w) => w.address),
        });
      }
    } catch (error) {
      if (id === generation) set({ isAssembling: false, lastError: describeError(error) });
    }
  },

  configureCache: async (config: Partial<Mic1CacheConfig>) => {
    const currentCpu = get().cpuState;
    const body = {
      state: currentCpu ?? undefined,
      config,
    };
    try {
      const res = await fetch("/api/sim/cache-config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
      });
      const data = await res.json();
      if (data.ok) {
        if (data.state) {
          set({
            cpuState: data.state,
            cacheState: data.cache ?? data.state.cache ?? null,
            lastError: null,
          });
        } else if (data.cache) {
          set({ cacheState: data.cache, lastError: null });
        }
      } else {
        set({ lastError: data.errorMessage || "Erro ao configurar a cache." });
      }
    } catch (err) {
      set({ lastError: describeError(err) });
    }
  },
}));
