import { InternalPanel } from "./InternalPanel";
import { useState } from "react";
import { BitRegister } from "./BitRegister";
import { ExecutionControls } from "./ExecutionControls";
import { useMic1Store } from "../store/mic1Store";
import { formatInstruction, toBinaryString } from "../types/mic1";
import { Win98Window } from "./win98/Win98Window";
import { Win98Sunken } from "./win98/Win98Input";

function comSinal16(n: number) {
  if (n >= 32768) return n - 65536;
  if (n < -32768) return n + 65536;
  return n;
}

// Janela 2: registradores e ciclos de execucao
export function ProcessorPanel() {
  const cpu = useMic1Store((s) => s.estadoCpu);
  const rodando = useMic1Store((s) => s.modoExecucao !== "idle");
  const executandoPasso = useMic1Store((s) => s.executandoPasso);
  const tempoExecucaoMs = useMic1Store((s) => s.tempoExecucaoMs);
  const [mostrarBits, setMostrarBits] = useState(false);

  const status = !cpu
    ? "A simulação ainda não começou"
    : cpu.errorMessage
    ? "ERRO"
    : cpu.halted
    ? "ENCERRADO"
    : rodando || executandoPasso
    ? "EXECUTANDO"
    : "PRONTO";

  const registers = cpu
    ? [
        { name: "PC", value: String(cpu.pc) },
        { name: "AC", value: String(comSinal16(cpu.ac)) },
        { name: "SP", value: String(comSinal16(cpu.sp)) },
        { name: "IR", value: toBinaryString(cpu.ir) },
        { name: "TIR", value: toBinaryString(cpu.registers?.[4] ?? 0) },
        { name: "A", value: String(comSinal16(cpu.registers?.[10] ?? 0)) },
        { name: "B", value: String(comSinal16(cpu.registers?.[11] ?? 0)) },
        { name: "C", value: String(comSinal16(cpu.registers?.[12] ?? 0)) },
        { name: "D", value: String(comSinal16(cpu.registers?.[13] ?? 0)) },
        { name: "E", value: String(comSinal16(cpu.registers?.[14] ?? 0)) },
        { name: "F", value: String(comSinal16(cpu.registers?.[15] ?? 0)) },
      ]
    : [];

  return (
    <Win98Window
      title="2 - Processador"
      headerRight={<span className="bg-white text-black px-1 text-[10px] font-bold">{status}</span>}
    >
      <div className="p-3 flex flex-col gap-3 h-full min-h-0 overflow-y-auto">
        <Win98Sunken className="p-3 flex items-center gap-3">
          <span className={`horse ${rodando ? "horse-running" : ""}`} aria-hidden="true">
            ♞
          </span>
          <div className="flex-1">
            <div className="flex items-baseline justify-between gap-3">
              <div>
                <p className="font-bold text-[22px] font-mono" data-testid="cycle-count">
                  {cpu?.cyclesExecuted ?? 0}
                </p>
                <p className="text-[12px] font-bold">Ciclos</p>
              </div>
              <div className="text-right">
                <p className="font-bold text-[18px] font-mono" data-testid="execution-time">
                  {tempoExecucaoMs}ms
                </p>
                <p className="text-[11px] text-win-faceDarker">Tempo de execução</p>
              </div>
            </div>
            <p className="text-[11px] mt-2 border-t border-win-faceLight pt-1 text-win-faceDarker">
              <span data-testid="instruction-count">{cpu?.instructionsExecuted ?? 0}</span> instruções ·{" "}
              <span data-testid="subcycle-count">{cpu?.subcyclesExecuted ?? 0}</span> subciclos
            </p>
          </div>
        </Win98Sunken>

        {!cpu ? (
          <p className="text-[13px] py-4 leading-relaxed">
            Escolha um exemplo ou escreva seu código. Clique em <b>Montar</b> ou <b>Gravar código na MP</b> para começar.
          </p>
        ) : (
          <>
            <div className="space-y-1">
              <p className="text-[12px] font-bold">Registradores</p>
              <div className="bg-white shadow-win-in max-h-48 overflow-y-auto">
                <table className="w-full text-[12px] font-mono border-collapse" data-testid="registers-table">
                  <thead className="sticky top-0 bg-win-face text-left border-b border-win-faceDark">
                    <tr>
                      <th className="px-2 py-1 w-20">Registrador</th>
                      <th className="px-2 py-1 text-right">Valor</th>
                    </tr>
                  </thead>
                  <tbody>
                    {registers.map((r) => (
                      <tr key={r.name} className="border-b border-win-faceLight/60 hover:bg-win-highlight/10">
                        <td className="px-2 py-0.5 font-bold">{r.name}</td>
                        <td className="px-2 py-0.5 text-right font-mono text-[11px]">{r.value}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            <label className="flex items-center gap-2 text-[12px] cursor-pointer">
              <input type="checkbox" checked={mostrarBits} onChange={(e) => setMostrarBits(e.target.checked)} /> Mostrar
              painel de bits (PC/AC/SP)
            </label>
            {mostrarBits && (
              <div className="space-y-1.5 border border-win-faceDark p-2 bg-win-faceLight/40">
                <BitRegister label="PC" value={cpu.pc} bitWidth={16} accent="warn" mostrarBits={true} description="Próxima instrução" />
                <BitRegister label="AC" value={cpu.ac} mostrarBits={true} description="Acumulador" />
                <BitRegister label="SP" value={cpu.sp} mostrarBits={true} description="Topo da pilha" />
              </div>
            )}

            <Win98Sunken className="p-2 text-[12px] space-y-1">
              <p>
                {cpu.atInstructionBoundary || cpu.halted ? "Última" : "Em execução"}:{" "}
                <b className="font-mono">
                  {!cpu.subcyclesExecuted
                    ? "—"
                    : !cpu.atInstructionBoundary &&
                      (cpu.currentMicroAddress < 2 || (cpu.currentMicroAddress === 2 && cpu.nextSubcycle !== 1))
                    ? "Buscando…"
                    : formatInstruction(cpu.ir)}
                </b>
              </p>
              <p>
                PC aponta para:{" "}
                <b className="font-mono">{cpu.halted ? "—" : formatInstruction(cpu.memory[cpu.pc & 4095])}</b>
              </p>
            </Win98Sunken>
          </>
        )}

        <ExecutionControls />
        {cpu && <InternalPanel cpu={cpu} />}
        <p className="text-[11px] text-win-faceDarker mt-auto pt-2">
          Uma microinstrução ocupa quatro subciclos. O cavalinho acompanha o ritmo.
        </p>
      </div>
    </Win98Window>
  );
}
