/**
 * EditorPanel: Janela 1 - entrada de assembly em texto, botoes separados
 * "Montar" e "Gravar código na MP" (F-13), preview do binario compilado
 * e exibicao da tabela de simbolos.
 */
import { useState } from "react";
import { useMic1Store } from "../store/mic1Store";
import { Win98Window } from "./win98/Win98Window";
import { Win98Button } from "./win98/Win98Button";
import { Win98TextArea, Win98Sunken } from "./win98/Win98Input";
import { resolveRtlEffect } from "../types/rtlEffects";
import { EXAMPLES } from "../store/examples";

export function EditorPanel() {
  const sourceCode = useMic1Store((s) => s.sourceCode);
  const setSourceCode = useMic1Store((s) => s.setSourceCode);
  const assembleOnly = useMic1Store((s) => s.assembleOnly);
  const loadProgramToMemory = useMic1Store((s) => s.loadProgramToMemory);
  const assemble = useMic1Store((s) => s.assemble);
  const isAssembling = useMic1Store((s) => s.isAssembling);
  const assembleResult = useMic1Store((s) => s.assembleResult);
  const cpuState = useMic1Store((s) => s.cpuState);
  const lastError = useMic1Store((s) => s.lastError);

  const [showSymbols, setShowSymbols] = useState(false);

  const symbols = assembleResult?.labelTable
    ? Object.entries(assembleResult.labelTable).map(([name, addr]) => ({ name, addr }))
    : [];

  return (
    <Win98Window title="1 - Editor.asm">
      <div className="p-2 flex flex-col gap-2 h-full min-h-0">
        <div className="flex flex-wrap gap-2 items-center">
          {/* Botoes independentes Montar e Gravar na MP (F-13) */}
          <Win98Button onClick={() => void assembleOnly()} disabled={isAssembling} title="Montar sem alterar a MP ou CPU">
            {isAssembling ? "Montando..." : "🔨 Montar"}
          </Win98Button>
          <Win98Button
            onClick={() => void loadProgramToMemory()}
            disabled={isAssembling || !assembleResult?.success}
            title="Gravar as instruções compiladas na memória principal sem zerar dados ou registradores"
          >
            💾 Gravar código na MP
          </Win98Button>
          <Win98Button onClick={() => void assemble()} disabled={isAssembling} title="Montar e inicializar CPU">
            ▶ Montar e iniciar
          </Win98Button>

          <select
            aria-label="Exemplos"
            value={EXAMPLES.findIndex((item) => item.source === sourceCode)}
            onChange={(e) => {
              const item = EXAMPLES[Number(e.target.value)];
              if (item) setSourceCode(item.source);
            }}
            className="bg-white shadow-win-in p-1 text-[12px] min-w-0 ml-auto"
          >
            <option value={-1} disabled>
              Personalizado
            </option>
            {EXAMPLES.map((item, index) => (
              <option key={item.name} value={index}>
                {item.name}
              </option>
            ))}
          </select>
        </div>

        <div className="flex-1 min-h-0 flex flex-col gap-2">
          {/* Textarea de codigo-fonte */}
          <Win98TextArea
            spellCheck={false}
            value={sourceCode}
            onChange={(e) => setSourceCode(e.target.value)}
            className="h-60 shrink-0 resize-y font-mono text-[13px] leading-relaxed"
            placeholder="LOCO 5&#10;STOD 100&#10;END: JUMP END"
            aria-label="Codigo assembly MIC-1"
          />

          {/* Secao de compilado e simbolos */}
          <div className="flex items-center justify-between text-[12px] px-1">
            <span className="font-bold">Compilado:</span>
            {symbols.length > 0 && (
              <label className="flex items-center gap-1 cursor-pointer text-[11px]">
                <input
                  type="checkbox"
                  checked={showSymbols}
                  onChange={(e) => setShowSymbols(e.target.checked)}
                />
                Ver tabela de símbolos ({symbols.length})
              </label>
            )}
          </div>

          {/* Tabela de simbolos (quando selecionada) */}
          {showSymbols && symbols.length > 0 && (
            <Win98Sunken className="max-h-28 overflow-y-auto p-1.5 bg-white">
              <table className="w-full text-[11px] font-mono border-collapse">
                <thead>
                  <tr className="border-b border-win-faceDark text-left">
                    <th className="px-1 py-0.5">Símbolo</th>
                    <th className="px-1 py-0.5 text-right">Endereço</th>
                  </tr>
                </thead>
                <tbody>
                  {symbols.map((s) => (
                    <tr key={s.name} className="border-b border-win-faceLight/60">
                      <td className="px-1 py-0.5 font-bold">{s.name}</td>
                      <td className="px-1 py-0.5 text-right">{s.addr}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </Win98Sunken>
          )}

          {/* Preview binario / erros */}
          <Win98Sunken className="flex-1 min-h-[140px] overflow-y-auto">
            {!assembleResult && (
              <div className="p-3 text-[13px] text-win-disabledText">
                Clique em <b>Montar</b> para compilar o código ou <b>Gravar código na MP</b> para carregá-lo.
              </div>
            )}

            {assembleResult && !assembleResult.success && (
              <div className="p-2 space-y-1.5">
                <p className="text-[13px] font-bold text-signal-danger px-1">
                  {assembleResult.errors.length} erro(s) de montagem:
                </p>
                {assembleResult.errors.map((err, i) => (
                  <div key={i} className="border border-signal-danger bg-white px-2 py-1.5">
                    <p className="text-[13px] text-signal-danger">
                      linha {err.lineNumber}: {err.message}
                    </p>
                    {err.sourceText && (
                      <p className="text-[12px] text-win-disabledText mt-0.5 truncate font-mono">{err.sourceText}</p>
                    )}
                  </div>
                ))}
              </div>
            )}

            {assembleResult && assembleResult.success && (
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
                  {assembleResult.program.map((w) => {
                    const rtl = resolveRtlEffect(w.mnemonic, w.operandDisplay);
                    return (
                      <tr
                        key={w.address}
                        className={`border-b border-win-faceDark/30 ${
                          cpuState &&
                          !cpuState.halted &&
                          (cpuState.atInstructionBoundary ? cpuState.pc & 4095 : cpuState.instructionAddress) ===
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

        {lastError && (
          <div role="alert" className="border border-signal-danger bg-white px-2 py-1.5">
            <p className="text-[13px] text-signal-danger">{lastError}</p>
          </div>
        )}
      </div>
    </Win98Window>
  );
}
