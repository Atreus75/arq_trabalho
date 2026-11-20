// Fieldset / groupbox estilo Win98
import type { ReactNode } from "react";

export function Win98GroupBox({ title, children, className = "" }: { title: string; children: ReactNode; className?: string }) {
  return (
    <fieldset className={`border-2 border-transparent relative pt-3 px-2 pb-2 ${className}`} style={{ boxShadow: "inset 1px 1px 0 0 #808080, inset -1px -1px 0 0 #FFFFFF" }}>
      <legend className="absolute -top-[9px] left-2 px-1 bg-win-face text-[13px] leading-none">{title}</legend>
      {children}
    </fieldset>
  );
}
