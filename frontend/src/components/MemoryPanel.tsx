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
const TAM_PAGINA = 32;

export function MemoryPanel() {
  const cpu = useMic1Store((s) => s.estadoCpu);
  const cache = useMic1Store((s) => s.estadoCache ?? s.estadoCpu?.cache);
  const gravados = useMic1Store((s) => s.enderecosGravados);
  const configurarCache = useMic1Store((s) => s.configurarCache);

  const [aba, setAba] = useState<MainTab>("memory");
  const [modo, setModo] = useState<ViewMode>("written");
  const [endereco, setEndereco] = useState(0);
  const [mostrarBinario, setMostrarBinario] = useState(false);

  // Formulario de configuracao da cache
  const [cfgAtiva, setCfgAtiva] = useState(true);
  const [cfgMapeamento, setCfgMapeamento] = useState<CacheMapping>("direct");
  const [cfgCapacidade, setCfgCapacidade] = useState(16);
  const [cfgTamBloco, setCfgTamBloco] = useState(2);
  const [cfgVias, setCfgVias] = useState(1);
  const [cfgSubstituicao, setCfgSubstituicao] = useState<CacheReplacement>("lru");
  const [cfgEscrita, setCfgEscrita] = useState<CacheWritePolicy>("write-through");
  const [cfgPenalidade, setCfgPenalidade] = useState(4);
  const [cfgStatus, setCfgStatus] = useState<string | null>(null);

  // Sincroniza campos quando cache do estado mudar
  useEffect(() => {
    if (cache) {
      setCfgAtiva(cache.enabled);
      setCfgMapeamento(cache.mapping);
      setCfgCapacidade(cache.capacityWords);
      setCfgTamBloco(cache.blockSizeWords);
      setCfgVias(cache.numWays);
      setCfgSubstituicao(cache.replacement);
      setCfgEscrita(cache.writePolicy);
      setCfgPenalidade(cache.penaltyCycles);
    }
  }, [cache]);

  const aplicarConfigCache = async () => {
    setCfgStatus("Aplicando...");
    await configurarCache({
      enabled: cfgAtiva,
      mapping: cfgMapeamento,
      capacityWords: cfgCapacidade,
      blockSizeWords: cfgTamBloco,
      numWays: cfgMapeamento === "direct" ? 1 : cfgVias,
      replacement: cfgSubstituicao,
      writePolicy: cfgEscrita,
      penaltyCycles: cfgPenalidade,
    });
    setCfgStatus("Configuração aplicada com sucesso!");
    setTimeout(() => setCfgStatus(null), 3000);
  };

  const enderecos = useMemo(() => {
    if (!cpu) return [];
    if (modo === "written") return gravados;
    let centro = endereco;
    if (modo === "program") {
      centro = cpu.pc & 4095;
    } else if (modo === "stack") {
      centro = cpu.sp === 4096 ? 4095 : cpu.sp & 0xfff;
    }
    const inicio = Math.max(0, Math.min(4096 - TAM_PAGINA, centro - (modo === "manual" ? 0 : 4)));
    return Array.from({ length: TAM_PAGINA }, (_, i) => inicio + i);
  }, [cpu, gravados, modo, endereco]);

  const porcentagemHit = cache?.stats?.hitRate !== undefined ? (cache.stats.hitRate * 100).toFixed(1) : "0.0";

  return (
    <Win98Window
      title="3 - Memória"
      headerRight={
        <div className="flex gap-1">
          <button
            onClick={() => setAba("memory")}
            className={`px-2 py-0.5 text-[11px] font-bold ${
              aba === "memory" ? "bg-white text-black" : "bg-win-face text-black opacity-80"
            }`}
          >
            Memória Principal
          </button>
          <button
            onClick={() => setAba("cache")}
            className={`px-2 py-0.5 text-[11px] font-bold ${
              aba === "cache" ? "bg-white text-black" : "bg-win-face text-black opacity-80"
            }`}
          >
            Cache {cache?.enabled ? "●" : "○"}
          </button>
        </div>
      }
    >
      <div className="p-3 flex flex-col gap-3 h-full min-h-0">
        {aba === "memory" ? (
          <>
            <div className="grid grid-cols-2 gap-1.5">
              <Win98Button active={modo === "written"} onClick={() => setModo("written")}>
                Gravados ({gravados.length})
              </Win98Button>
              <Win98Button active={modo === "program"} onClick={() => setModo("program")}>
                Programa (PC)
              </Win98Button>
              <Win98Button active={modo === "stack"} onClick={() => setModo("stack")}>
                Pilha (SP)
              </Win98Button>
              <Win98Button active={modo === "manual"} onClick={() => setModo("manual")}>
                Ir ao endereço
              </Win98Button>
            </div>

            {modo === "manual" && (
              <label className="flex gap-2 items-center text-[12px]">
                Endereço:
                <Win98Input
                  type="number"
                  min={0}
                  max={4095}
                  value={endereco}
                  className="w-24"
                  onChange={(e) =>
                    setEndereco(Math.max(0, Math.min(4095, Math.trunc(Number(e.target.value)) || 0)))
                  }
                />
              </label>
            )}

            <div className="flex-1 min-h-0 overflow-auto bg-white shadow-win-in">
              {enderecos.length === 0 ? (
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
                      {mostrarBinario && <th className="px-2 py-1.5">Binário</th>}
                    </tr>
                  </thead>
                  <tbody>
                    {enderecos.map((end) => {
                      const valor = cpu!.memory[end];
                      // Enderecos da pilha ficam em azul (end >= SP)
                      const ehPilha = cpu!.sp < 4096 && end >= (cpu!.sp & 0xfff);
                      // Endereco em cinza e para onde o PC aponta
                      const ehPc = end === (cpu!.pc & 4095);

                      let classeLinha = "border-b border-win-faceLight";
                      let tag = "";
                      if (ehPilha) {
                        classeLinha += " bg-[#0000cc] text-white font-bold";
                        if (end === (cpu!.sp & 0xfff)) tag = "SP";
                      } else if (ehPc) {
                        classeLinha += " bg-gray-600 text-white font-bold";
                        tag = "PC";
                      }

                      return (
                        <tr key={end} data-address={end} className={classeLinha}>
                          <td className="px-2 py-1">
                            {end} {tag && <small className="ml-1 opacity-90">[{tag}]</small>}
                          </td>
                          <td className="px-2 py-1 text-right font-bold" data-value={valor}>
                            {valor}
                          </td>
                          <td className="px-2 py-1 text-right">{toHexString(valor)}</td>
                          {mostrarBinario && (
                            <td className="px-2 py-1 whitespace-nowrap text-right">{toBinaryString(valor)}</td>
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
                <input type="checkbox" checked={mostrarBinario} onChange={(e) => setMostrarBinario(e.target.checked)} />
                Mostrar binário
              </label>
              <span>4.096 palavras (0..4095)</span>
            </div>
            <p className="text-[11px] text-win-faceDarker italic">
              Endereços em azul são parte da pilha. Endereço em cinza é para onde o PC aponta no momento.
            </p>
          </>
        ) : (
          <div className="flex flex-col gap-3 h-full min-h-0 overflow-y-auto">
            <div className="border border-win-faceDark p-2.5 bg-win-faceLight/30 space-y-2 text-[12px]">
              <div className="flex items-center justify-between font-bold border-b border-win-faceDark/40 pb-1">
                <span>Configuração da Cache</span>
                <label className="flex items-center gap-1 font-normal cursor-pointer text-[12px]">
                  <input
                    type="checkbox"
                    checked={cfgAtiva}
                    onChange={(e) => setCfgAtiva(e.target.checked)}
                  />
                  Habilitada
                </label>
              </div>

              <div className="grid grid-cols-2 gap-2">
                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Mapeamento:</span>
                  <select
                    value={cfgMapeamento}
                    onChange={(e) => {
                      const m = e.target.value as CacheMapping;
                      setCfgMapeamento(m);
                      if (m === "direct") setCfgVias(1);
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
                    value={cfgCapacidade}
                    onChange={(e) => setCfgCapacidade(Number(e.target.value))}
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
                    value={cfgTamBloco}
                    onChange={(e) => setCfgTamBloco(Number(e.target.value))}
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
                    value={cfgVias}
                    disabled={cfgMapeamento === "direct" || cfgMapeamento === "fully"}
                    onChange={(e) => setCfgVias(Number(e.target.value))}
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
                    value={cfgSubstituicao}
                    onChange={(e) => setCfgSubstituicao(e.target.value as CacheReplacement)}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value="lru">LRU (Least Recently Used)</option>
                    <option value="fifo">FIFO (First In First Out)</option>
                  </select>
                </label>

                <label className="flex flex-col gap-0.5">
                  <span className="text-[11px]">Política de Escrita:</span>
                  <select
                    value={cfgEscrita}
                    onChange={(e) => setCfgEscrita(e.target.value as CacheWritePolicy)}
                    className="bg-white shadow-win-in p-1 text-[11px]"
                  >
                    <option value="write-through">Write-Through</option>
                    <option value="write-back">Write-Back</option>
                  </select>
                </label>
              </div>

              <div className="flex items-center justify-between pt-1">
                <span className="text-[11px] text-win-faceDarker">
                  Penalidade miss: <b>{cfgPenalidade} ciclos</b>
                </span>
                <Win98Button onClick={aplicarConfigCache} className="font-bold">
                  Aplicar Configuração
                </Win98Button>
              </div>
              {cfgStatus && <p className="text-[11px] text-green-700 font-bold">{cfgStatus}</p>}
            </div>

            <div className="border border-win-faceDark p-2.5 bg-white shadow-win-in space-y-2 text-[12px]">
              <p className="font-bold border-b border-win-faceLight pb-1">Métricas & Desempenho</p>

              <div>
                <div className="flex justify-between items-baseline mb-1">
                  <span className="text-[11px] font-bold">Taxa de Acertos (Hit Rate):</span>
                  <span className="font-mono font-bold text-[13px]">{porcentagemHit}%</span>
                </div>
                <div className="w-full bg-win-faceLight h-4 shadow-win-in border border-win-faceDark overflow-hidden">
                  <div
                    className="bg-win-highlight h-full transition-all duration-300"
                    style={{ width: `${Math.min(100, Math.max(0, Number(porcentagemHit)))}%` }}
                  />
                </div>
              </div>

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
                        const ehUltimoAcesso =
                          cache.lastAccess?.hasAccess &&
                          line.set === cache.lastAccess.setIndex &&
                          line.way === cache.lastAccess.wayIndex;

                        let classeLinha = "border-b border-win-faceLight/60 hover:bg-win-faceLight/30";
                        if (ehUltimoAcesso) {
                          classeLinha = cache.lastAccess.hit
                            ? "bg-green-100 font-bold border-l-4 border-green-700"
                            : "bg-red-100 font-bold border-l-4 border-red-700";
                        }

                        return (
                          <tr key={line.index} className={classeLinha}>
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
