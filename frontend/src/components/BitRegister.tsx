/**
 * BitRegister: exibe um registrador de 16 bits como uma fileira de
 * indicadores retangulares (estilo painel de controle clássico: LED
 * apagado = ranhura entalhada cinza; LED aceso = quadrado colorido
 * sólido), além dos valores decimal (com sinal) e hexadecimal.
 */
import { toUnsigned16, toHexString } from "../types/mic1";
import { Win98GroupBox } from "./win98/Win98GroupBox";

interface BitRegisterProps {
  label: string;
  value: number;
  bitWidth?: 12 | 16;
  accent?: "data" | "control" | "warn";
  description?: string;
  showBits?: boolean;
}

const ACCENT_BG: Record<NonNullable<BitRegisterProps["accent"]>, string> = {
  data: "bg-signal-data",
  control: "bg-signal-control",
  warn: "bg-signal-warn",
};

export function BitRegister({ label, value, bitWidth = 16, accent = "data", description, showBits = false }: BitRegisterProps) {
  const unsigned = toUnsigned16(value);
  const bits = unsigned.toString(2).padStart(bitWidth, "0").split("");

  return (
    <Win98GroupBox title={label}>
      <div title={description} className="flex flex-col gap-1">
        <div className="flex items-baseline justify-between gap-2 -mt-0.5 mb-1">
          <span data-testid={`register-${label}`} className="font-mono text-[22px] font-bold text-black">{value}</span>
          <span className="font-mono text-[11px] text-win-disabledText">0x{toHexString(value)}</span>
        </div>
        <p className="text-[11px] text-win-faceDarker">{description}</p>
        {showBits && <div className="flex gap-[2px]" role="img" aria-label={`${label} = ${value} em binario ${bits.join("")}`}>
          {bits.map((bit, i) => (
            <span
              key={i}
              className={`flex-1 aspect-square ${
                bit === "1" ? `${ACCENT_BG[accent]} shadow-win-in-1` : "bg-win-face shadow-win-in-1"
              }`}
            />
          ))}
        </div>}
      </div>
    </Win98GroupBox>
  );
}
