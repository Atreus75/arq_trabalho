// Janela 1: editor de assembly, montagem e preview de instrucoes
import { useState } from "react";
import { useMic1Store } from "../store/mic1Store";
import { Win98Window } from "./win98/Win98Window";
import { Win98Button } from "./win98/Win98Button";
import { Win98TextArea, Win98Sunken } from "./win98/Win98Input";
import { resolveRtlEffect } from "../types/rtlEffects";
import { EXEMPLOS } from "../store/examples";

export function EditorPanel() {
  const codigoFonte = useMic1Store((s) => s.codigoFonte);
  const setCodigoFonte = useMic1Store((s) => s.setCodigoFonte);
  const apenasMontar = useMic1Store((s) => s.apenasMontar);
  const gravarNaMemoria = useMic1Store((s) => s.gravarNaMemoria);
  const montarEIniciar = useMic1Store((s) => s.montarEIniciar);
  const montando = useMic1Store((s) => s.montando);
  const resultadoMontagem = useMic1Store((s) => s.resultadoMontagem);
  const estadoCpu = useMic1Store((s) => s.estadoCpu);
  const ultimoErro = useMic1Store((s) => s.ultimoErro);

  const [mostrarSimbolos, setMostrarSimbolos] = useState(false);

  const simbolos = resultadoMontagem?.labelTable
    ? Object.entries(resultadoMontagem.labelTable).map(([name, end]) => ({ name, end }))
    : [];

  return (
    <Win98Window title="1 - Editor.asm">
      <div className="p-2 flex flex-col gap-2 h-full min-h-0">
        <div className="flex flex-wrap gap-2 items-center">
          <Win98Button onClick={() => void apenasMontar()} disabled={montando} title="Montar sem alterar a MP ou CPU">
            {montando ? "Montando..." : "🔨 Montar"}
          </Win98Button>
          <Win98Button
            onClick={() => void gravarNaMemoria()}
            disabled={montando || !resultadoMontagem?.success}
            title="Gravar as instruções compiladas na memória principal sem zerar dados ou registradores"
          >
            💾 Gravar código na MP
          </Win98Button>
          <Win98Button onClick={() => void montarEIniciar()} disabled={montando} title="Montar e inicializar CPU">
            ▶ Montar e iniciar
          </Win98Button>

          <select
            aria-label="Exemplos"
            value={EXEMPLOS.findIndex((item) => item.source === codigoFonte)}
            onChange={(e) => {
              const item = EXEMPLOS[Number(e.target.value)];
              if (item) setCodigoFonte(item.source);
            }}
            className="bg-white shadow-win-in p-1 text-[12px] min-w-0 ml-auto"
          >
            <option value={-1} disabled>
              Personalizado
            </option>
            {EXEMPLOS.map((item, indice) => (
              <option key={item.name} value={indice}>
                {item.name}
              </option>
            ))}
          </select>
        </div>

        <div className="flex-1 min-h-0 flex flex-col gap-2">
          <Win98TextArea
            spellCheck={false}
            value={codigoFonte}
            onChange={(e) => setCodigoFonte(e.target.value)}
            className="h-60 shrink-0 resize-y font-mono text-[13px] leading-relaxed"
            placeholder="LOCO 5&#10;STOD 100&#10;END: JUMP END"
            aria-label="Codigo assembly MIC-1"
          />

          <div className="flex items-center justify-between text-[12px] px-1">
            <span className="font-bold">Compilado:</span>
            {simbolos.length > 0 && (
              <label className="flex items-center gap-1 cursor-pointer text-[11px]">
                <input
                  type="checkbox"
                  checked={mostrarSimbolos}
                  onChange={(e) => setMostrarSimbolos(e.target.checked)}
                />
                Ver tabela de símbolos ({simbolos.length})
              </label>
            )}
          </div>

          {mostrarSimbolos && simbolos.length > 0 && (
            <Win98Sunken className="max-h-28 overflow-y-auto p-1.5 bg-white">
              <table className="w-full text-[11px] font-mono border-collapse">
                <thead>
                  <tr className="border-b border-win-faceDark text-left">
                    <th className="px-1 py-0.5">Símbolo</th>
                    <th className="px-1 py-0.5 text-right">Endereço</th>
                  </tr>
                </thead>
                <tbody>
                  {simbolos.map((s) => (
                    <tr key={s.name} className="border-b border-win-faceLight/60">
                      <td className="px-1 py-0.5 font-bold">{s.name}</td>
                      <td className="px-1 py-0.5 text-right">{s.end}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </Win98Sunken>
          )}

          <Win98Sunken className="flex-1 min-h-[140px] overflow-y-auto">
            {!resultadoMontagem && (
              <div className="p-3 text-[13px] text-win-disabledText">
                Clique em <b>Montar</b> para compilar o código ou <b>Gravar código na MP</b> para carregá-lo.
              </div>
            )}

            {resultadoMontagem && !resultadoMontagem.success && (
              <div className="p-2 space-y-1.5">
                <p className="text-[13px] font-bold text-signal-danger px-1">
                  {resultadoMontagem.errors.length} erro(s) de montagem:
                </p>
                {resultadoMontagem.errors.map((erro, i) => (
                  <div key={i} className="border border-signal-danger bg-white px-2 py-1.5">
                    <p className="text-[13px] text-signal-danger">
                      linha {erro.lineNumber}: {erro.message}
                    </p>
                    {erro.sourceText && (
                      <p className="text-[12px] text-win-disabledText mt-0.5 truncate font-mono">{erro.sourceText}</p>
                    )}
                  </div>
                ))}
              </div>
            )}

            {resultadoMontagem && resultadoMontagem.success && (
              <table className="w-full text-[12px] font-mono border-collapse">
                <thead className="sticky top-0 bg-win-face">
                  <tr className="text-black">
                    <th className="text-left font-bold px-2 py-1 border-b-2 border-win-faceDark w-10">end</th>
                    <th className="text-left font-bold px-2 py-1 border-b-2 border-win-faceDark w-[136px]">binário</th>
                    <th className="text-left font-bold px-2 py-1 border-b-2 border-win-faceDark whitespace-nowrap">
                      instrução
                    </th>
                  </tr>
                </thead>
                <tbody>
                  {resultadoMontagem.program.map((w) => {
                    const rtl = resolveRtlEffect(w.mnemonic, w.operandDisplay);
                    return (
                      <tr
                        key={w.address}
                        className={`border-b border-win-faceDark/30 ${
                          estadoCpu &&
                          !estadoCpu.halted &&
                          (estadoCpu.atInstructionBoundary ? estadoCpu.pc & 4095 : estadoCpu.instructionAddress) ===
                            w.address
                            ? "bg-signal-warn/20"
                            : ""
                        }`}
                      >
                        <td className="px-2 py-1 text-win-disabledText">{w.address}</td>
                        <td className="px-2 py-1 text-signal-data whitespace-nowrap">{w.binaryString}</td>
                        <td className="px-2 py-1 font-bold text-black whitespace-nowrap" title={rtl}>
                          {w.mnemonic}{" "}
                          <span className="font-normal text-win-disabledText">
                            {w.operandDisplay !== "(sem operando)"
                              ? w.operandDisplay
                                  .replace(/\s*\(label.*?\)/, "")
                                  .replace(/→.*/, "")
                                  .trim()
                              : ""}
                          </span>
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            )}
          </Win98Sunken>
        </div>

        {ultimoErro && (
          <div role="alert" className="border border-signal-danger bg-white px-2 py-1.5">
            <p className="text-[13px] text-signal-danger">{ultimoErro}</p>
          </div>
        )}
      </div>
    </Win98Window>
  );
}
