import { useMemo, useState, useEffect } from "react";
import { useMic1Store } from "../store/mic1Store";
import {
  toBinaryString,
  toHexString,
  type CacheMapping,
  type CacheReplacement,
  type CacheWritePolicy,
} from "../types/mic1";
import { Win98Window } from "./win98/Win98Window";
import { Win98Button } from "./win98/Win98Button";
import { Win98Input } from "./win98/Win98Input";

type MainTab = "memory" | "cache";
type ViewMode = "program" | "written" | "stack" | "manual";
const PAGE_SIZE = 32;

export function MemoryPanel() {
  const cpu = useMic1Store((s) => s.cpuState);
  const cache = useMic1Store((s) => s.cacheState ?? s.cpuState?.cache);
  const written = useMic1Store((s) => s.writtenAddresses);
  const configureCache = useMic1Store((s) => s.configureCache);

  const [mainTab, setMainTab] = useState<MainTab>("memory");
  const [mode, setMode] = useState<ViewMode>("written");
  const [address, setAddress] = useState(0);
  const [showBinary, setShowBinary] = useState(false);

  // Formulario de configuracao da cache
  const [cfgEnabled, setCfgEnabled] = useState(true);
  const [cfgMapping, setCfgMapping] = useState<CacheMapping>("direct");
  const [cfgCapacity, setCfgCapacity] = useState(16);
  const [cfgBlockSize, setCfgBlockSize] = useState(2);
  const [cfgWays, setCfgWays] = useState(1);
  const [cfgReplacement, setCfgReplacement] = useState<CacheReplacement>("lru");
  const [cfgWritePolicy, setCfgWritePolicy] = useState<CacheWritePolicy>("write-through");
  const [cfgPenalty, setCfgPenalty] = useState(4);
  const [cfgStatus, setCfgStatus] = useState<string | null>(null);

  // Sincroniza campos quando cache do estado mudar
  useEffect(() => {
    if (cache) {
      setCfgEnabled(cache.enabled);
      setCfgMapping(cache.mapping);
      setCfgCapacity(cache.capacityWords);
      setCfgBlockSize(cache.blockSizeWords);
      setCfgWays(cache.numWays);
      setCfgReplacement(cache.replacement);
      setCfgWritePolicy(cache.writePolicy);
      setCfgPenalty(cache.penaltyCycles);
    }
  }, [cache]);

  const handleApplyCacheConfig = async () => {
    setCfgStatus("Aplicando...");
    await configureCache({
      enabled: cfgEnabled,
      mapping: cfgMapping,
      capacityWords: cfgCapacity,
      blockSizeWords: cfgBlockSize,
      numWays: cfgMapping === "direct" ? 1 : cfgWays,
      replacement: cfgReplacement,
      writePolicy: cfgWritePolicy,
      penaltyCycles: cfgPenalty,
    });
    setCfgStatus("Configuração aplicada com sucesso!");
    setTimeout(() => setCfgStatus(null), 3000);
  };

  const addresses = useMemo(() => {
    if (!cpu) return [];
    if (mode === "written") return written;
    const center =
      mode === "program"
        ? cpu.pc & 4095
        : mode === "stack"
        ? cpu.sp === 4096
          ? 4095
          : cpu.sp & 0xfff
        : address;
    const start = Math.max(0, Math.min(4096 - PAGE_SIZE, center - (mode === "manual" ? 0 : 4)));
    return Array.from({ length: PAGE_SIZE }, (_, i) => start + i);
  }, [cpu, written, mode, address]);

  const hitRatePercent = cache?.stats?.hitRate !== undefined ? (cache.stats.hitRate * 100).toFixed(1) : "0.0";

  return (
    <Win98Window
      title="3 - Memória"
      headerRight={
        <div className="flex gap-1">
          <button
            onClick={() => setMainTab("memory")}
            className={`px-2 py-0.5 text-[11px] font-bold ${
              mainTab === "memory" ? "bg-white text-black" : "bg-win-face text-black opacity-80"
            }`}
          >
            Memória Principal
          </button>
          <button
            onClick={() => setMainTab("cache")}
            className={`px-2 py-0.5 text-[11px] font-bold ${
              mainTab === "cache" ? "bg-white text-black" : "bg-win-face text-black opacity-80"
            }`}
          >
            Cache {cache?.enabled ? "●" : "○"}
          </button>
        </div>
      }
    >
      <div className="p-3 flex flex-col gap-3 h-full min-h-0">
        {mainTab === "memory" ? (
          <>
            {/* Controles de visualizacao de memoria */}
            <div className="grid grid-cols-2 gap-1.5">
              <Win98Button active={mode === "written"} onClick={() => setMode("written")}>
                Gravados ({written.length})
              </Win98Button>
              <Win98Button active={mode === "program"} onClick={() => setMode("program")}>
                Programa (PC)
              </Win98Button>
              <Win98Button active={mode === "stack"} onClick={() => setMode("stack")}>
                Pilha (SP)
              </Win98Button>
              <Win98Button active={mode === "manual"} onClick={() => setMode("manual")}>
                Ir ao endereço
              </Win98Button>
            </div>

            {mode === "manual" && (
              <label className="flex gap-2 items-center text-[12px]">
                Endereço:
                <Win98Input
                  type="number"
                  min={0}
                  max={4095}
                  value={address}
                  className="w-24"
                  onChange={(e) =>
                    setAddress(Math.max(0, Math.min(4095, Math.trunc(Number(e.target.value)) || 0)))
                  }
                />
              </label>
            )}

            {/* Tabela de Memoria com destaque continuo de Pilha (F-16) e PC */}
            <div className="flex-1 min-h-0 overflow-auto bg-white shadow-win-in">
              {addresses.length === 0 ? (
                <p className="text-[13px] p-5 leading-relaxed">
                  {!cpu
                    ? "Monte e inicie um programa para acompanhar a memória."
                    : "Nenhuma gravação ainda. Este painel também mostra valores gravados como zero."}
                </p>
              ) : (
                <table className="w-full text-[12px] font-mono border-collapse" data-testid="memory-table">
                  <thead className="sticky top-0 bg-win-face">
                    <tr>
                      <th className="text-left px-2 py-1.5">Endereço</th>
                      <th className="text-right px-2 py-1.5">Valor</th>
                      <th className="text-right px-2 py-1.5">Hex</th>
                      {showBinary && <th className="px-2 py-1.5">Binário</th>}
                    </tr>
                  </thead>
                  <tbody>
                    {addresses.map((addr) => {
                      const value = cpu!.memory[addr];
                      // F-16: Enderecos em azul sao parte da pilha (addr >= SP quando SP < 4096)
                      const isStack = cpu!.sp < 4096 && addr >= (cpu!.sp & 0xfff);
                      // Endereco em cinza e para onde o PC aponta
                      const isPc = addr === (cpu!.pc & 4095);

                      let rowClass = "border-b border-win-faceLight";
                      let badge = "";
                      if (isStack) {
                        rowClass += " bg-[#0000cc] text-white font-bold";
                        if (addr === (cpu!.sp & 0xfff)) badge = "SP";
                      } else if (isPc) {
                        rowClass += " bg-gray-600 text-white font-bold";
                        badge = "PC";
                      }

                      return (
                        <tr key={addr} data-address={addr} className={rowClass}>
                          <td className="px-2 py-1">
                            {addr} {badge && <small className="ml-1 opacity-90">[{badge}]</small>}
                          </td>
                          <td className="px-2 py-1 text-right font-bold" data-value={value}>
                            {value}
                          </td>
                          <td className="px-2 py-1 text-right">{toHexString(value)}</td>
                          {showBinary && (
                            <td className="px-2 py-1 whitespace-nowrap text-right">{toBinaryString(value)}</td>
                          )}
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              )}
            </div>

            <div className="flex items-center justify-between text-[11px] text-win-faceDarker">
              <label className="flex items-center gap-2 cursor-pointer">
                <input type="checkbox" checked={showBinary} onChange={(e) => setShowBinary(e.target.checked)} />
                Mostrar binário
              </label>
              <span>4.096 palavras (0..4095)</span>
            </div>
            <p className="text-[11px] text-win-faceDarker italic">
              Endereços em azul são parte da pilha. Endereço em cinza é para onde o PC aponta no momento.
            </p>
          </>
        ) : (
          /* Aba dedicada de Memoria Cache */
          <div className="flex flex-col gap-3 h-full min-h-0 overflow-y-auto">
            {/* Painel de Configuracao */}
            <div className="border border-win-faceDark p-2.5 bg-win-faceLight/30 space-y-2 text-[12px]">
              <div className="flex items-center justify-between font-bold border-b border-win-faceDark/40 pb-1">
                <span>Configuração da Cache</span>
                <label className="flex items-center gap-1 font-normal cursor-pointer text-[12px]">
                  <input
                    type="checkbox"
                    checked={cfgEnabled}
                    onChange={(e) => setCfgEnabled(e.target.checked)}
                  />
                  Habilitada
                </label>
              </div>

              <div className="grid grid-cols-2 gap-2">
                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Mapeamento:</span>
                  <select
                    value={cfgMapping}
                    onChange={(e) => {
                      const m = e.target.value as CacheMapping;
                      setCfgMapping(m);
                      if (m === "direct") setCfgWays(1);
                    }}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value="direct">Direto (1 via)</option>
                    <option value="set">Associativo por Conjunto</option>
                    <option value="fully">Totalmente Associativo</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Capacidade (palavras):</span>
                  <select
                    value={cfgCapacity}
                    onChange={(e) => setCfgCapacity(Number(e.target.value))}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value={8}>8 palavras</option>
                    <option value={16}>16 palavras</option>
                    <option value={32}>32 palavras</option>
                    <option value={64}>64 palavras</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Tam. Bloco (palavras):</span>
                  <select
                    value={cfgBlockSize}
                    onChange={(e) => setCfgBlockSize(Number(e.target.value))}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value={1}>1 palavra</option>
                    <option value={2}>2 palavras</option>
                    <option value={4}>4 palavras</option>
                    <option value={8}>8 palavras</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Vias por conjunto:</span>
                  <select
                    value={cfgWays}
                    disabled={cfgMapping === "direct" || cfgMapping === "fully"}
                    onChange={(e) => setCfgWays(Number(e.target.value))}
                    className="bg-white shadow-win-in p-1 text-[11px] disabled:opacity-50"
                  >
                    <option value={1}>1 via</option>
                    <option value={2}>2 vias</option>
                    <option value={4}>4 vias</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Substituição:</span>
                  <select
                    value={cfgReplacement}
                    onChange={(e) => setCfgReplacement(e.target.value as CacheReplacement)}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value="lru">LRU (Least Recently Used)</option>
                    <option value="fifo">FIFO (First In First Out)</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Política de Escrita:</span>
                  <select
                    value={cfgWritePolicy}
                    onChange={(e) => setCfgWritePolicy(e.target.value as CacheWritePolicy)}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value="write-through">Write-Through</option>
                    <option value="write-back">Write-Back</option>
                  </select>
                </label>
              </div>

              <div className="flex items-center justify-between pt-1">
                <span className="text-[11px] text-win-faceDarker">
                  Penalidade miss: <b>{cfgPenalty} ciclos</b>
                </span>
                <Win98Button onClick={handleApplyCacheConfig} className="font-bold">
                  Aplicar Configuração
                </Win98Button>
              </div>
              {cfgStatus && <p className="text-[11px] text-green-700 font-bold">{cfgStatus}</p>}
            </div>

            {/* Dashboard de Metricas */}
            <div className="border border-win-faceDark p-2.5 bg-white shadow-win-in space-y-2 text-[12px]">
              <p className="font-bold border-b border-win-faceLight pb-1">Métricas & Desempenho</p>

              {/* Barra de progresso de Hit Rate */}
              <div>
                <div className="flex justify-between items-baseline mb-1">
                  <span className="text-[11px] font-bold">Taxa de Acertos (Hit Rate):</span>
                  <span className="font-mono font-bold text-[13px]">{hitRatePercent}%</span>
                </div>
                <div className="w-full bg-win-faceLight h-4 shadow-win-in border border-win-faceDark overflow-hidden">
                  <div
                    className="bg-win-highlight h-full transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, Number(hitRatePercent)))}%` }}
                  />
                </div>
              </div>

              {/* Grid de contadores */}
              <div className="grid grid-cols-2 gap-x-3 gap-y-1 text-[11px] font-mono">
                <div>
                  Leituras: <b>{cache?.stats?.readHits ?? 0}</b> hits / <b>{cache?.stats?.readMisses ?? 0}</b> misses
                </div>
                <div>
                  Escritas: <b>{cache?.stats?.writeHits ?? 0}</b> hits / <b>{cache?.stats?.writeMisses ?? 0}</b> misses
                </div>
                <div>
                  Total: <b>{cache?.stats?.totalHits ?? 0}</b> hits / <b>{cache?.stats?.totalMisses ?? 0}</b> misses
                </div>
                <div>
                  Penalidade total: <b>{cache?.stats?.penaltyCyclesTotal ?? 0}</b> ciclos
                </div>
              </div>

              {/* Indicador do Ultimo Acesso */}
              <div className="border-t border-win-faceLight pt-1.5 flex items-center gap-2">
                <span className="text-[11px] font-bold">Último acesso:</span>
                {cache?.lastAccess?.hasAccess ? (
                  <div className="flex items-center gap-2 text-[11px] font-mono">
                    <span
                      className={`px-1.5 py-0.5 text-white font-bold rounded-none ${
                        cache.lastAccess.hit ? "bg-green-700" : "bg-red-700"
                      }`}
                    >
                      {cache.lastAccess.hit ? "[HIT]" : "[MISS]"}
                    </span>
                    <span>
                      {cache.lastAccess.event} @ end. <b>{cache.lastAccess.address}</b> (bloco {cache.lastAccess.block},
                      conj. {cache.lastAccess.setIndex}, via {cache.lastAccess.wayIndex}, tag {cache.lastAccess.tag})
                    </span>
                  </div>
                ) : (
                  <span className="text-[11px] text-win-disabledText">Nenhum acesso registrado ainda.</span>
                )}
              </div>
            </div>

            {/* Tabela de Linhas da Cache */}
            <div className="space-y-1 flex-1 min-h-[140px] flex flex-col">
              <p className="text-[12px] font-bold">
                Linhas da Cache ({cache?.lines?.length ?? 0} linhas, {cache?.numSets ?? 0} conjuntos)
              </p>
              <div className="flex-1 min-h-0 overflow-auto bg-white shadow-win-in">
                {!cache?.lines || cache.lines.length === 0 ? (
                  <p className="text-[12px] p-3 text-win-disabledText">Nenhuma linha de cache disponível.</p>
                ) : (
                  <table className="w-full text-[11px] font-mono border-collapse">
                    <thead className="sticky top-0 bg-win-face border-b border-win-faceDark">
                      <tr className="text-left">
                        <th className="px-1 py-1">Idx</th>
                        <th className="px-1 py-1">Conj.</th>
                        <th className="px-1 py-1">Via</th>
                        <th className="px-1 py-1">Vál.</th>
                        <th className="px-1 py-1">Sujo</th>
                        <th className="px-1 py-1">Tag</th>
                        <th className="px-1 py-1">End.Bloco</th>
                        <th className="px-1 py-1">Palavras</th>
                        <th className="px-1 py-1 text-right">Acesso</th>
                      </tr>
                    </thead>
                    <tbody>
                      {cache.lines.map((line) => {
                        const isLastAccessLine =
                          cache.lastAccess?.hasAccess &&
                          line.set === cache.lastAccess.setIndex &&
                          line.way === cache.lastAccess.wayIndex;

                        const rowClass = isLastAccessLine
                          ? cache.lastAccess.hit
                            ? "bg-green-100 font-bold border-l-4 border-green-700"
                            : "bg-red-100 font-bold border-l-4 border-red-700"
                          : "border-b border-win-faceLight/60 hover:bg-win-faceLight/30";

                        return (
                          <tr key={line.index} className={rowClass}>
                            <td className="px-1 py-0.5">{line.index}</td>
                            <td className="px-1 py-0.5">{line.set}</td>
                            <td className="px-1 py-0.5">{line.way}</td>
                            <td className="px-1 py-0.5">{line.valid ? "1" : "0"}</td>
                            <td className="px-1 py-0.5">{line.dirty ? "1" : "0"}</td>
                            <td className="px-1 py-0.5">{line.valid ? line.tag : "—"}</td>
                            <td className="px-1 py-0.5">{line.valid ? line.blockAddress : "—"}</td>
                            <td className="px-1 py-0.5 truncate max-w-[120px]" title={line.words.join(", ")}>
                              [{line.words.slice(0, 4).join(",")}{line.words.length > 4 ? "…" : ""}]
                            </td>
                            <td className="px-1 py-0.5 text-right">{line.lastAccessTime}</td>
                          </tr>
                        );
                      })}
                    </tbody>
                  </table>
                )}
              </div>
            </div>
          </div>
        )}
      </div>
    </Win98Window>
  );
}
