// Layout principal com as 3 janelas (Editor, Processador, Memoria)
import { useState, useEffect } from "react";
import { EditorPanel } from "./components/EditorPanel";
import { ProcessorPanel } from "./components/ProcessorPanel";
import { MemoryPanel } from "./components/MemoryPanel";

function useClock() {
  const [agora, setAgora] = useState(new Date());
  useEffect(() => {
    const id = setInterval(() => setAgora(new Date()), 1000 * 15);
    return () => clearInterval(id);
  }, []);
  return agora;
}

function App() {
  const agora = useClock();
  const horaTexto = agora.toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" });

  return (
    <div className="min-h-screen flex flex-col bg-win-desktop">
      <main className="flex-1 p-3 pb-1">
        <div className="h-full grid grid-cols-1 lg:grid-cols-[1.1fr_0.85fr_1fr] gap-3">
          <div className="min-w-0 h-[min(75vh,650px)] lg:h-[calc(100vh-64px)]">
            <EditorPanel />
          </div>
          <div className="min-w-0 h-[min(85vh,780px)] lg:h-[calc(100vh-64px)]">
            <ProcessorPanel />
          </div>
          <div className="min-w-0 h-[min(75vh,650px)] lg:h-[calc(100vh-64px)]">
            <MemoryPanel />
          </div>
        </div>
      </main>

      {/* barra de tarefas decorativa */}
      <footer className="h-9 shrink-0 bg-win-face shadow-win-out-1 flex items-center justify-between px-1.5 gap-2">
        <div
          className="h-7 px-3 flex items-center gap-1.5 bg-win-face shadow-win-out active:shadow-win-in text-[13px] font-bold select-none"
        >
          <span className="text-signal-control">♞</span> MIC-1.sim
        </div>
        <div className="flex-1 flex items-center gap-1.5 overflow-hidden">
          <span className="h-7 px-3 flex items-center bg-win-face shadow-win-in text-[12px] text-win-disabledText select-none truncate">
            Arquitetura de Computadores · 16 bits
          </span>
        </div>
        <span className="h-7 px-3 flex items-center bg-win-face shadow-win-in text-[12px] select-none shrink-0">{horaTexto}</span>
      </footer>
    </div>
  );
}

export default App;
