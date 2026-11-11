/** Moldura simples com o visual classico do Windows. */
import type { ReactNode } from "react";

interface Win98WindowProps {
  title: string;
  icon?: ReactNode;
  children: ReactNode;
  headerRight?: ReactNode; // conteúdo extra à direita do título (ex: badge de status)
  className?: string;
}

export function Win98Window({ title, icon, children, headerRight, className = "" }: Win98WindowProps) {
  return (
    <section
      className={`flex flex-col h-full bg-win-face shadow-win-out ${className}`}
      style={{ padding: "2px" }}
    >
      {/* barra de titulo */}
      <header
        className="flex items-center justify-between px-1 py-[3px] shrink-0"
        style={{
          background: "linear-gradient(90deg, #000080 0%, #1084D0 100%)",
        }}
      >
        <div className="flex items-center gap-1.5 min-w-0">
          {icon && <span className="shrink-0 leading-none">{icon}</span>}
          <span className="text-white text-[13px] font-bold truncate select-none">{title}</span>
        </div>
        <div className="flex items-center gap-2 shrink-0">
          {headerRight}

        </div>
      </header>

      {/* corpo da janela */}
      <div className="flex-1 min-h-0 bg-win-face flex flex-col mt-[2px]">{children}</div>
    </section>
  );
}
