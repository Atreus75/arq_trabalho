// Efeito RTL de cada instrucao para exibir no editor

const MODELOS_RTL: Record<string, string> = {
  LODD: "ac := m[{x}]",
  STOD: "m[{x}] := ac",
  ADDD: "ac := ac + m[{x}]",
  SUBD: "ac := ac - m[{x}]",
  JPOS: "se ac ≥ 0, pc := {x}",
  JZER: "se ac = 0, pc := {x}",
  JUMP: "pc := {x}",
  LOCO: "ac := {x}",
  LODL: "ac := m[sp+{x}]",
  STOL: "m[sp+{x}] := ac",
  ADDL: "ac := ac + m[sp+{x}]",
  SUBL: "ac := ac - m[sp+{x}]",
  JNEG: "se ac < 0, pc := {x}",
  JNZE: "se ac ≠ 0, pc := {x}",
  CALL: "pilha; pc := {x}",
  PSHI: "empilha m[ac]",
  POPI: "m[ac] := topo da pilha",
  PUSH: "empilha ac",
  POP: "ac := topo da pilha",
  RETN: "retorna (pc := pilha)",
  SWAP: "troca ac ↔ sp",
  INSP: "sp := sp + {y}",
  DESP: "sp := sp - {y}",
};

// Substitui {x} e {y} pelo valor numerico do operando
export function resolveRtlEffect(mnemonico: string, textoOperando: string): string {
  const modelo = MODELOS_RTL[mnemonico];
  if (!modelo) return "";

  // textoOperando chega como "300", "4 (label 'CONTADOR')", "(sem operando)",
  // Extrai o numero inicial, quando houver operando.
  const achou = textoOperando.match(/-?\d+/);
  const valorNumerico = achou ? achou[0] : "";

  return modelo.replace("{x}", valorNumerico).replace("{y}", valorNumerico);
}
