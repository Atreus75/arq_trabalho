import { useEffect, useState } from "react";
import type { Mic1CpuState } from "../types/mic1";
import { toHexString } from "../types/mic1";

const REGISTER_NAMES = ["PC", "AC", "SP", "IR", "TIR", "0", "1", "−1", "AMASK", "SMASK", "A", "B", "C", "D", "E", "F"];
const SUBCYCLES = ["Buscar microinstrução", "Capturar entradas", "ULA e deslocador", "Gravar e decidir salto"];
const signed = (n: number) => n >= 32768 ? n - 65536 : n;
type RomRow = { address: number; word: number; text: string };

export function InternalPanel({ cpu }: { cpu: Mic1CpuState }) {
  const [open, setOpen] = useState(false);
  const [rom, setRom] = useState<RomRow[]>([]);
  const [romError, setRomError] = useState("");
  useEffect(() => {
    if (!open || rom.length) return;
    const abort = new AbortController();
    fetch("/api/microcode", { signal: abort.signal }).then((r) => {
      if (!r.ok) throw new Error("Não foi possível carregar a memória de controle.");
      return r.json();
    }).then((data) => setRom(data.microcode)).catch((err: Error) => { if (err.name !== "AbortError") setRomError(err.message); });
    return () => abort.abort();
  }, [open, rom.length]);
  const fields = [
    ["AMUX", 31, 1], ["COND", 29, 3], ["ALU", 27, 3], ["SH", 25, 3],
    ["MBR", 24, 1], ["MAR", 23, 1], ["RD", 22, 1], ["WR", 21, 1], ["ENC", 20, 1],
    ["C", 16, 15], ["B", 12, 15], ["A", 8, 15], ["ADDR", 0, 255],
  ] as const;
  return <details open={open} onToggle={(e) => setOpen(e.currentTarget.open)} className="border border-win-faceDark p-2 text-[12px]">
    <summary className="font-bold cursor-pointer">Ver funcionamento interno</summary>
    <div className="pt-3 space-y-3" data-testid="internal-panel">
      <p>Próximo subciclo: <b data-testid="next-subcycle">{cpu.nextSubcycle}</b> · MPC: <b>{cpu.mpc}</b></p>
      <ol className="grid grid-cols-2 gap-1">
        {SUBCYCLES.map((label, i) => <li key={label} className={`p-1.5 border ${!cpu.halted && cpu.nextSubcycle === i + 1 ? "bg-win-highlight text-white border-win-highlight" : "bg-white border-win-faceDark"}`}>{i + 1}. {label}</li>)}
      </ol>
      <div className="bg-white shadow-win-in p-2 break-words">
        <p>Microinstrução carregada: <b>{cpu.subcyclesExecuted ? cpu.currentMicroAddress : "—"}</b></p>
        <p className="font-mono mt-1">{cpu.subcyclesExecuted ? cpu.microInstruction : "Aguardando o primeiro passo."}</p>
        <p className="mt-1">MIR: <code>0x{cpu.mir.toString(16).padStart(8, "0").toUpperCase()}</code></p>
      </div>
      <div className="flex flex-wrap gap-1 font-mono" aria-label="Campos da microinstrução">
        {fields.map(([name, shift, mask]) => <span key={name} className="border border-win-faceDark px-1 bg-white">{name}={(cpu.mir >>> shift) & mask}</span>)}
      </div>
      <table className="w-full font-mono text-[11px] border-collapse">
        <thead><tr><th className="text-left">Registrador</th><th className="text-right">Decimal</th><th className="text-right">Hex</th></tr></thead>
        <tbody>{[
          ...cpu.registers.map((value, i) => ({ name: REGISTER_NAMES[i], value })),
          { name: "MAR", value: cpu.mar },
          { name: "MBR", value: cpu.mbr & 65535 },
          { name: "MBR (Read)", value: (cpu.mbrRead ?? cpu.mbr ?? 0) & 65535 },
          { name: "MBR (Write)", value: (cpu.mbrWrite ?? cpu.mbr ?? 0) & 65535 },
        ].map(({ name, value }) =>
          <tr key={name} className="border-b border-win-faceDark/30"><td className="py-1">{name}</td><td className="text-right">{name === "PC" || name === "MAR" ? value : signed(value)}</td><td className="text-right">{toHexString(value)}</td></tr>)}</tbody>
      </table>
      <div className="bg-white shadow-win-in p-2 space-y-1 font-mono">
        <p>Latch A: {signed(cpu.latchA)} · B: {signed(cpu.latchB)}</p>
        <p>Entrada A da ULA: {signed(cpu.aluA)}</p>
        <p>Saída ULA: {signed(cpu.aluOut)} · N={Number(cpu.negative)} Z={Number(cpu.zero)}</p>
        <p>Deslocador: {signed(cpu.shiftOut)}</p>
        <p>Memória: {cpu.memoryPending === 1 ? `lendo m[${cpu.pendingAddress}]` : cpu.memoryPending === 2 ? `gravando m[${cpu.pendingAddress}]` : cpu.readReady ? "leitura pronta para o MBR" : "livre"}</p>
      </div>
      <details><summary className="cursor-pointer font-bold">Histórico do último passo ({cpu.trace.length} subciclos)</summary>
        <ol className="mt-2 max-h-56 overflow-auto bg-white shadow-win-in p-2 font-mono text-[11px] space-y-2">
          {cpu.trace.map((entry, i) => <li key={i}><b>µ{entry.mpc}</b> · {entry.label}</li>)}
        </ol>
      </details>
      <details><summary className="cursor-pointer font-bold">Memória de controle ({rom.length || "…"}/79 palavras)</summary>
        {romError && <p>{romError}</p>}
        <div className="mt-2 max-h-56 overflow-auto bg-white shadow-win-in">
          <table className="w-full font-mono text-[11px]">
            <thead><tr><th>µ</th><th className="text-left">Microinstrução</th></tr></thead>
            <tbody>{rom.map((row) => <tr key={row.address} title={`0x${row.word.toString(16).padStart(8, "0")}`} className={cpu.subcyclesExecuted && row.address === cpu.currentMicroAddress ? "bg-win-highlight text-white" : ""}><td className="p-1">{row.address}</td><td className="p-1 break-words">{row.text}</td></tr>)}</tbody>
          </table>
        </div>
      </details>
    </div>
  </details>;
}
