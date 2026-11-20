import { useState, useEffect } from "react";
import { useMic1Store } from "../store/mic1Store";
import type { StepUnit } from "../types/mic1";
import { Win98Button } from "./win98/Win98Button";
import { Win98GroupBox } from "./win98/Win98GroupBox";

export function ExecutionControls() {
  const {
    estadoCpu,
    modoExecucao,
    executandoPasso,
    montando,
    pausaMs,
    setPausaMs,
    passoSelecionado,
    iniciarExecucao,
    pausar,
    reiniciar,
    unidadePasso,
    setUnidadePasso,
    breakpoint,
    breakpointAtivado,
    aplicarBreakpoint,
    motivoPausa,
  } = useMic1Store();

  const [textoBp, setTextoBp] = useState<string>(breakpoint !== null ? String(breakpoint) : "");

  useEffect(() => {
    if (breakpoint === null) setTextoBp("");
    else setTextoBp(String(breakpoint));
  }, [breakpoint]);

  const rodando = modoExecucao !== "idle";
  const disabled = !estadoCpu || estadoCpu.halted || montando;

  const aplicarBp = () => {
    const limpo = textoBp.trim();
    if (limpo === "") {
      aplicarBreakpoint(null);
    } else {
      const valor = Math.min(4095, Math.max(0, Math.trunc(Number(limpo)) || 0));
      aplicarBreakpoint(valor);
    }
  };

  return (
    <Win98GroupBox title="Controles da Simulação">
      <div className="grid grid-cols-2 gap-2">
        <Win98Button onClick={iniciarExecucao} disabled={disabled || rodando || executandoPasso}>
          ▶ Executar
        </Win98Button>
        <Win98Button onClick={pausar} disabled={!rodando}>
          Ⅱ Pausar
        </Win98Button>
        <Win98Button onClick={() => void passoSelecionado()} disabled={disabled || rodando || executandoPasso}>
          → Passo ({unidadePasso === "cycle" ? "Ciclo" : unidadePasso === "instruction" ? "Instrução" : "Subciclo"})
        </Win98Button>
        <Win98Button onClick={() => void reiniciar()} disabled={!estadoCpu || montando}>
          ↺ Reiniciar
        </Win98Button>
      </div>

      <div className="mt-3 space-y-2 text-[12px]">
        <label className="flex items-center justify-between gap-2">
          Modo do passo:
          <select
            aria-label="Unidade do passo"
            value={unidadePasso}
            disabled={rodando || executandoPasso}
            onChange={(e) => setUnidadePasso(e.target.value as StepUnit)}
            className="bg-white shadow-win-in p-1 font-mono"
          >
            <option value="cycle">Ciclo (padrão)</option>
            <option value="instruction">Instrução</option>
            <option value="subcycle">Subciclo</option>
          </select>
        </label>

        <label className="flex items-center justify-between gap-2">
          Pausa entre passos (ms):
          <div className="flex items-center gap-1">
            <input
              type="number"
              min={50}
              max={5000}
              step={50}
              value={pausaMs}
              onChange={(e) => setPausaMs(Number(e.target.value))}
              className="w-20 bg-white shadow-win-in p-1 font-mono text-right"
              aria-label="Pausa entre passos em milissegundos"
            />
            <span className="text-[11px] text-win-faceDarker">ms</span>
          </div>
        </label>

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
                value={textoBp}
                onChange={(e) => setTextoBp(e.target.value)}
                onKeyDown={(e) => {
                  if (e.key === "Enter") aplicarBp();
                }}
              />
              <Win98Button onClick={aplicarBp} className="px-2 py-0.5 text-[11px]">
                Aplicar
              </Win98Button>
            </div>
          </label>
          {breakpoint !== null && (
            <p className="text-[11px] text-win-faceDarker mt-1 text-right">
              Breakpoint armado em <b>PC = {breakpoint}</b> {breakpointAtivado ? "(disparado)" : "(aguardando)"}
            </p>
          )}
        </div>
      </div>

      {motivoPausa && (
        <div role="status" className="border border-win-highlight bg-win-faceLight/60 p-1.5 text-[12px] mt-2">
          {motivoPausa}
        </div>
      )}
    </Win98GroupBox>
  );
}
