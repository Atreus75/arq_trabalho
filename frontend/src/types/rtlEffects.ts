/**
 * Efeitos RTL (register transfer level) de cada instrução MIC-1, para
 * exibição didática ao lado do binário no Editor. O objetivo é deixar
 * INEQUÍVOCO que instruções como ADDD, SUBD, LODD, STOD operam sobre o
 * CONTEÚDO de um endereço de memória (m[x]), nunca sobre o literal x —
 * essa é a fonte mais comum de confusão ao escrever assembly MIC-1
 * (ex: escrever "ADDD 30" esperando somar o número 30, quando na
 * verdade soma o que estiver armazenado em m[30]).
 *
 * Os templates usam {x} / {y} para o operando já resolvido (endereço
 * decimal, não o label original) — ver resolveRtlEffect abaixo.
 */

const RTL_TEMPLATES: Record<string, string> = {
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

/**
 * Resolve o efeito RTL de uma linha já montada, substituindo {x}/{y}
 * pelo valor numérico do operando (extraído de operandDisplay, que já
 * vem resolvido do backend — labels viram número + anotação).
 */
export function resolveRtlEffect(mnemonic: string, operandDisplay: string): string {
  const template = RTL_TEMPLATES[mnemonic];
  if (!template) return "";

  // operandDisplay chega como "300", "4 (label 'CONTADOR')", "(sem operando)",
  // Extrai o numero inicial, quando houver operando.
  const match = operandDisplay.match(/-?\d+/);
  const numericValue = match ? match[0] : "";

  return template.replace("{x}", numericValue).replace("{y}", numericValue);
}
