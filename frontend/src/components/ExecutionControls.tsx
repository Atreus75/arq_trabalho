import { useState, useEffect } from "react";
import { useMic1Store } from "../store/mic1Store";
import type { StepUnit } from "../types/mic1";
import { Win98Button } from "./win98/Win98Button";
import { Win98GroupBox } from "./win98/Win98GroupBox";

export function ExecutionControls() {
  const {
    cpuState,
    runMode,
    isStepping,
    isAssembling,
    delayMs,
    setDelayMs,
    stepSelected,
    startRunMacro,
    pause,
    reset,
    stepUnit,
    setStepUnit,
    breakpoint,
    breakpointTriggered,
    applyBreakpoint,
    pauseReason,
  } = useMic1Store();

  const [bpInput, setBpInput] = useState<string>(breakpoint !== null ? String(breakpoint) : "");

  useEffect(() => {
    if (breakpoint === null) setBpInput("");
    else setBpInput(String(breakpoint));
  }, [breakpoint]);

  const running = runMode !== "idle";
  const disabled = !cpuState || cpuState.halted || isAssembling;

  const handleApplyBreakpoint = () => {
    const trimmed = bpInput.trim();
    if (trimmed === "") {
      applyBreakpoint(null);
    } else {
      const parsed = Math.min(4095, Math.max(0, Math.trunc(Number(trimmed)) || 0));
      applyBreakpoint(parsed);
    }
  };

  return (
    <Win98GroupBox title="Controles da Simulação">
      <div className="grid grid-cols-2 gap-2">
        <Win98Button onClick={startRunMacro} disabled={disabled || running || isStepping}>
          ▶ Executar
        </Win98Button>
        <Win98Button onClick={pause} disabled={!running}>
          Ⅱ Pausar
        </Win98Button>
        <Win98Button onClick={() => void stepSelected()} disabled={disabled || running || isStepping}>
          → Passo ({stepUnit === "cycle" ? "Ciclo" : stepUnit === "instruction" ? "Instrução" : "Subciclo"})
        </Win98Button>
        <Win98Button onClick={() => void reset()} disabled={!cpuState || isAssembling}>
          ↺ Reiniciar
        </Win98Button>
      </div>

      <div className="mt-3 space-y-2 text-[12px]">
        {/* Unidade do passo (F-08: padrao Ciclo) */}
        <label className="flex items-center justify-between gap-2">
          Modo do passo:
          <select
            aria-label="Unidade do passo"
            value={stepUnit}
            disabled={running || isStepping}
            onChange={(e) => setStepUnit(e.target.value as StepUnit)}
            className="bg-white shadow-win-in p-1 font-mono"
          >
            <option value="cycle">Ciclo (padrão)</option>
            <option value="instruction">Instrução</option>
            <option value="subcycle">Subciclo</option>
          </select>
        </label>

        {/* Delay milissegundos (F-08: 50ms..5000ms, padrao 1000ms) */}
        <label className="flex items-center justify-between gap-2">
          Pausa entre passos (ms):
          <div className="flex items-center gap-1">
            <input
              type="number"
              min={50}
              max={5000}
              step={50}
              value={delayMs}
              onChange={(e) => setDelayMs(Number(e.target.value))}
              className="w-20 bg-white shadow-win-in p-1 font-mono text-right"
              aria-label="Pausa entre passos em milissegundos"
            />
            <span className="text-[11px] text-win-faceDarker">ms</span>
          </div>
        </label>

        {/* Breakpoint PC com botao Aplicar e disparo unico (F-09) */}
        <div className="pt-1 border-t border-win-faceLight">
          <label className="flex items-center justify-between gap-2">
            <span>Pausar quando PC:</span>
            <div className="flex items-center gap-1">
              <input
                aria-label="Pausar automaticamente quando PC"
                className="w-16 bg-white shadow-win-in p-1 font-mono text-right"
                type="number"
                min={0}
                max={4095}
                placeholder="—"
                value={bpInput}
                onChange={(e) => setBpInput(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === "Enter") handleApplyBreakpoint();
                }}
              />
              <Win98Button onClick={handleApplyBreakpoint} className="px-2 py-0.5 text-[11px]">
                Aplicar
              </Win98Button>
            </div>
          </label>
          {breakpoint !== null && (
            <p className="text-[11px] text-win-faceDarker mt-1 text-right">
              Breakpoint armado em <b>PC = {breakpoint}</b> {breakpointTriggered ? "(disparado)" : "(aguardando)"}
            </p>
          )}
        </div>
      </div>

      {pauseReason && (
        <div role="status" className="border border-win-highlight bg-win-faceLight/60 p-1.5 text-[12px] mt-2">
          {pauseReason}
        </div>
      )}
    </Win98GroupBox>
  );
}
