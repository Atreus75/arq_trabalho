// Botao estilo Win98
import type { ButtonHTMLAttributes } from "react";

interface Win98ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  active?: boolean; // força o estado "pressionado" (ex: modo em execução)
}

export function Win98Button({ active = false, className = "", children, disabled, ...rest }: Win98ButtonProps) {
  return (
    <button
      type="button"
      disabled={disabled}
      className={`
        px-3 py-1 text-[13px] bg-win-face text-black select-none
        ${active ? "shadow-win-in" : "shadow-win-out active:shadow-win-in"}
        ${disabled ? "text-win-disabledText" : ""}
        ${className}
      `}
      {...rest}
    >
      <span className={active ? "inline-block translate-x-px translate-y-px" : "inline-block active:translate-x-px active:translate-y-px"}>
        {children}
      </span>
    </button>
  );
}
