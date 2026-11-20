// Inputs estilo Win98
import type { TextareaHTMLAttributes, InputHTMLAttributes } from "react";

export function Win98TextArea({ className = "", ...rest }: TextareaHTMLAttributes<HTMLTextAreaElement>) {
  return (
    <textarea
      className={`bg-white text-black shadow-win-in outline-none p-1.5 ${className}`}
      {...rest}
    />
  );
}

export function Win98Input({ className = "", ...rest }: InputHTMLAttributes<HTMLInputElement>) {
  return (
    <input
      className={`bg-white text-black shadow-win-in outline-none px-1.5 py-0.5 ${className}`}
      {...rest}
    />
  );
}

export function Win98Sunken({ className = "", children }: { className?: string; children: React.ReactNode }) {
  return <div className={`bg-white shadow-win-in ${className}`}>{children}</div>;
}
