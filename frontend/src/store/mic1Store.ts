/** Estado da interface. O motor C continua sendo o unico executor. */
import { create } from "zustand";
import type { AssembleResult, Mic1CpuState, Mic1CacheState, Mic1CacheConfig, StepUnit } from "../types/mic1";
import { assembleProgram, simInit, simStepMacro, simStepMicro, simStepSubcycle } from "../api/client";
import { EXEMPLOS } from "./examples";

export const VELOCIDADES_CLOCK = [
  { label: "Lento", intervalMs: 1000 },
  { label: "Normal", intervalMs: 350 },
  { label: "Rápido", intervalMs: 60 },
] as const;

let geracao = 0;
function descreverErro(erro: unknown) {
  if (erro instanceof Error) return erro.message;
  return String(erro);
}

interface Mic1Store {
  codigoFonte: string;
  resultadoMontagem: AssembleResult | null;
  estadoCpu: Mic1CpuState | null;
  estadoCache: Mic1CacheState | null;
  enderecosGravados: number[];
  montando: boolean;
  cpuIniciada: boolean;
  executandoPasso: boolean;
  ultimoErro: string | null;
  modoExecucao: "idle" | "running-macro";
  indiceVelocidade: number;
  pausaMs: number;
  tempoExecucaoMs: number;
  contagemInicio: number;
  _timer: ReturnType<typeof setInterval> | null;
  unidadePasso: StepUnit;
  breakpoint: number | null;
  breakpointAtivado: boolean;
  motivoPausa: string | null;

  setCodigoFonte: (code: string) => void;
  montarEIniciar: () => Promise<void>;
  apenasMontar: () => Promise<void>;
  gravarNaMemoria: () => Promise<void>;
  setUnidadePasso: (unidade: StepUnit) => void;
  setBreakpoint: (address: number | null) => void;
  aplicarBreakpoint: (address: number | null) => void;
  setPausaMs: (ms: number) => void;
  setIndiceVelocidade: (indice: number) => void;
  passoSelecionado: () => Promise<void>;
  _passo: (unidade: StepUnit) => Promise<void>;
  passoInstrucao: () => Promise<void>;
  iniciarExecucao: () => void;
  pausar: () => void;
  reiniciar: () => Promise<void>;
  configurarCache: (config: Partial<Mic1CacheConfig>) => Promise<void>;
}

export const useMic1Store = create<Mic1Store>((set, get) => ({
  codigoFonte: EXEMPLOS[0].source,
  resultadoMontagem: null,
  estadoCpu: null,
  estadoCache: null,
  enderecosGravados: [],
  montando: false,
  cpuIniciada: false,
  executandoPasso: false,
  ultimoErro: null,
  modoExecucao: "idle",
  indiceVelocidade: 0,
  pausaMs: 1000,
  tempoExecucaoMs: 0,
  contagemInicio: 0,
  unidadePasso: "cycle",
  breakpoint: null,
  breakpointAtivado: false,
  motivoPausa: null,
  _timer: null,

  setUnidadePasso: (unidadePasso) => { get().pausar(); set({ unidadePasso }); },
  setBreakpoint: (breakpoint) => set({ breakpoint }),
  aplicarBreakpoint: (breakpoint) => set({ breakpoint, breakpointAtivado: false, motivoPausa: null }),

  setPausaMs: (pausaMs: number) => {
    const limitado = Math.max(50, Math.min(5000, Math.trunc(pausaMs) || 1000));
    set({ pausaMs: limitado });
    if (get().modoExecucao === "running-macro") {
      const timer = get()._timer;
      if (timer !== null) clearInterval(timer);
      let ultimoTempo = Date.now();
      const novoTimer = setInterval(() => {
        const agora = Date.now();
        const delta = agora - ultimoTempo;
        ultimoTempo = agora;
        set((s) => ({ tempoExecucaoMs: s.tempoExecucaoMs + delta }));
        void get().passoSelecionado();
      }, limitado);
      set({ _timer: novoTimer });
    }
  },

  setIndiceVelocidade: (indice) => {
    if (!Number.isInteger(indice) || indice < 0 || indice >= VELOCIDADES_CLOCK.length) return;
    const ms = VELOCIDADES_CLOCK[indice].intervalMs;
    set({ indiceVelocidade: indice, pausaMs: ms });
    if (get().modoExecucao === "running-macro") {
      const timer = get()._timer;
      if (timer !== null) clearInterval(timer);
      let ultimoTempo = Date.now();
      const novoTimer = setInterval(() => {
        const agora = Date.now();
        const delta = agora - ultimoTempo;
        ultimoTempo = agora;
        set((s) => ({ tempoExecucaoMs: s.tempoExecucaoMs + delta }));
        void get().passoSelecionado();
      }, ms);
      set({ _timer: novoTimer });
    }
  },

  passoSelecionado: () => get()._passo(get().unidadePasso),
  passoInstrucao: () => get()._passo("instruction"),

  setCodigoFonte: (codigoFonte) => {
    geracao++;
    get().pausar();
    set({
      codigoFonte,
      resultadoMontagem: null,
      estadoCpu: null,
      estadoCache: null,
      enderecosGravados: [],
      montando: false,
      executandoPasso: false,
      cpuIniciada: false,
      motivoPausa: null,
      ultimoErro: null,
      tempoExecucaoMs: 0,
      breakpointAtivado: false,
    });
  },

  apenasMontar: async () => {
    get().pausar();
    const id = ++geracao;
    const codigoFonte = get().codigoFonte;
    set({ montando: true, resultadoMontagem: null, motivoPausa: null, ultimoErro: null });
    try {
      const resultado = await assembleProgram({ sourceCode: codigoFonte });
      if (id !== geracao) return;
      set({ resultadoMontagem: resultado, montando: false });
    } catch (erro) {
      if (id === geracao) set({ montando: false, ultimoErro: descreverErro(erro) });
    }
  },

  gravarNaMemoria: async () => {
    const resultado = get().resultadoMontagem;
    if (!resultado?.success || !resultado.program.length) {
      set({ ultimoErro: "Monte o programa com sucesso antes de gravar na MP." });
      return;
    }
    const atual = get().estadoCpu;
    if (!atual) {
      const id = ++geracao;
      set({ montando: true, ultimoErro: null, motivoPausa: null });
      try {
        const estadoCpu = await simInit(resultado.program.map((word) => word.word));
        if (id === geracao) {
          set({
            estadoCpu,
            estadoCache: estadoCpu.cache ?? null,
            montando: false,
            cpuIniciada: true,
            enderecosGravados: resultado.program.map((w) => w.address),
            tempoExecucaoMs: 0,
            breakpointAtivado: false,
          });
        }
      } catch (erro) {
        if (id === geracao) set({ montando: false, ultimoErro: descreverErro(erro) });
      }
      return;
    }

    const novaMemoria = [...atual.memory];
    const gravados = new Set(get().enderecosGravados);
    for (const w of resultado.program) {
      novaMemoria[w.address] = w.word;
      gravados.add(w.address);
    }
    const novoEstado: Mic1CpuState = {
      ...atual,
      memory: novaMemoria,
    };
    set({
      estadoCpu: novoEstado,
      enderecosGravados: [...gravados].sort((a, b) => a - b),
      ultimoErro: null,
    });
  },

  montarEIniciar: async () => {
    get().pausar();
    const id = ++geracao;
    const codigoFonte = get().codigoFonte;
    set({
      montando: true,
      executandoPasso: false,
      estadoCpu: null,
      estadoCache: null,
      resultadoMontagem: null,
      cpuIniciada: false,
      enderecosGravados: [],
      motivoPausa: null,
      ultimoErro: null,
      tempoExecucaoMs: 0,
      breakpointAtivado: false,
    });
    try {
      const resultado = await assembleProgram({ sourceCode: codigoFonte });
      if (id !== geracao) return;
      set({ resultadoMontagem: resultado });
      if (!resultado.success) { set({ montando: false }); return; }
      const estadoCpu = await simInit(resultado.program.map((word) => word.word));
      if (id !== geracao) return;
      set({
        estadoCpu,
        estadoCache: estadoCpu.cache ?? null,
        montando: false,
        cpuIniciada: true,
        enderecosGravados: resultado.program.map((w) => w.address),
      });
    } catch (erro) {
      if (id === geracao) set({ montando: false, ultimoErro: descreverErro(erro) });
    }
  },

  _passo: async (unidade) => {
    const { estadoCpu, executandoPasso, montando } = get();
    if (!estadoCpu || estadoCpu.halted || executandoPasso || montando) return;
    const id = geracao;
    set({ executandoPasso: true, ultimoErro: null, motivoPausa: null });
    try {
      let proximo: Mic1CpuState;
      if (unidade === "instruction") {
        proximo = await simStepMacro(estadoCpu);
      } else if (unidade === "cycle") {
        proximo = await simStepMicro(estadoCpu);
      } else {
        proximo = await simStepSubcycle(estadoCpu);
      }
      if (id !== geracao) return;

      const gravados = new Set(get().enderecosGravados);
      for (const item of proximo.trace) {
        if (item.writeAddress >= 0) gravados.add(item.writeAddress);
      }
      set({
        estadoCpu: proximo,
        estadoCache: proximo.cache ?? get().estadoCache,
        executandoPasso: false,
        enderecosGravados: [...gravados].sort((a, b) => a - b),
        ultimoErro: proximo.errorMessage || null,
      });

      if (proximo.halted) {
        get().pausar();
      } else if (get().modoExecucao !== "idle" && get().breakpoint !== null && !get().breakpointAtivado) {
        if ((proximo.pc & 4095) === get().breakpoint) {
          get().pausar();
          set({
            breakpointAtivado: true,
            motivoPausa: `Pausa no endereço ${proximo.pc} (Breakpoint atingido). Clique em Executar para continuar.`,
          });
        }
      }
    } catch (erro) {
      if (id !== geracao) return;
      set({ executandoPasso: false, ultimoErro: descreverErro(erro) });
      get().pausar();
    }
  },

  iniciarExecucao: () => {
    const state = get();
    if (state.modoExecucao !== "idle" || !state.estadoCpu || state.estadoCpu.halted || state.montando || state.executandoPasso) return;
    let ultimoTempo = Date.now();
    const timer = setInterval(() => {
      const agora = Date.now();
      const delta = agora - ultimoTempo;
      ultimoTempo = agora;
      set((s) => ({ tempoExecucaoMs: s.tempoExecucaoMs + delta }));
      void get().passoSelecionado();
    }, state.pausaMs);
    set({
      modoExecucao: "running-macro",
      _timer: timer,
      contagemInicio: state.estadoCpu.instructionsExecuted,
    });
    void get().passoSelecionado();
  },

  pausar: () => {
    const timer = get()._timer;
    if (timer !== null) clearInterval(timer);
    set({ modoExecucao: "idle", _timer: null });
  },

  reiniciar: async () => {
    get().pausar();
    const resultado = get().resultadoMontagem;
    if (!resultado?.success) return;
    const id = ++geracao;
    set({
      montando: true,
      executandoPasso: false,
      motivoPausa: null,
      ultimoErro: null,
      estadoCpu: null,
      estadoCache: null,
      cpuIniciada: false,
      enderecosGravados: [],
      tempoExecucaoMs: 0,
      breakpointAtivado: false,
    });
    try {
      const estadoCpu = await simInit(resultado.program.map((word) => word.word));
      if (id === geracao) {
        set({
          estadoCpu,
          estadoCache: estadoCpu.cache ?? null,
          montando: false,
          cpuIniciada: true,
          enderecosGravados: resultado.program.map((w) => w.address),
        });
      }
    } catch (erro) {
      if (id === geracao) set({ montando: false, ultimoErro: descreverErro(erro) });
    }
  },

  configurarCache: async (config: Partial<Mic1CacheConfig>) => {
    const cpuAtual = get().estadoCpu;
    const corpo = {
      state: cpuAtual ?? undefined,
      config,
    };
    try {
      const res = await fetch("/api/sim/cache-config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(corpo),
      });
      const dados = await res.json();
      if (dados.ok) {
        if (dados.state) {
          set({
            estadoCpu: dados.state,
            estadoCache: dados.cache ?? dados.state.cache ?? null,
            ultimoErro: null,
          });
        } else if (dados.cache) {
          set({ estadoCache: dados.cache, ultimoErro: null });
        }
      } else {
        set({ ultimoErro: dados.errorMessage || "Erro ao configurar a cache." });
      }
    } catch (erro) {
      set({ ultimoErro: descreverErro(erro) });
    }
  },
}));
