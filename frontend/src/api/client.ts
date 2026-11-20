// Chamadas HTTP para o simulador
import type { AssembleResult, Mic1CpuState } from "../types/mic1";

// Em dev o Vite faz proxy de /api pro backend
const BASE_API = "/api";

class ApiError extends Error {
  status?: number;
  constructor(mensagem: string, status?: number) {
    super(mensagem);
    this.name = "ApiError";
    this.status = status;
  }
}

async function postJson<T>(rota: string, corpo: unknown): Promise<T> {
  let resposta: Response;
  try {
    resposta = await fetch(`${BASE_API}${rota}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(corpo),
    });
  } catch (erro) {
    throw new ApiError(
      `Nao foi possivel conectar ao back-end em ${BASE_API}${rota}. Verifique se o simulador esta aberto. (${
        erro instanceof Error ? erro.message : String(erro)
      })`
    );
  }

  let json: unknown;
  try {
    json = await resposta.json();
  } catch {
    throw new ApiError(`O back-end respondeu com um corpo que nao e JSON valido (status ${resposta.status}).`, resposta.status);
  }

  if (!resposta.ok) {
    const mensagem =
      json && typeof json === "object" && "errorMessage" in json
        ? String((json as { errorMessage: unknown }).errorMessage)
        : `Erro HTTP ${resposta.status}`;
    throw new ApiError(mensagem, resposta.status);
  }

  return json as T;
}

export interface AssembleRequest {
  sourceCode: string;
  programName?: string;
}

export async function assembleProgram(req: AssembleRequest): Promise<AssembleResult> {
  return postJson<AssembleResult>("/assemble", req);
}

interface SimResponse {
  ok: boolean;
  state?: Mic1CpuState;
  errorMessage?: string;
}

async function callSim(rota: string, corpo: unknown): Promise<Mic1CpuState> {
  const resp = await postJson<SimResponse>(rota, corpo);
  if (!resp.ok || !resp.state) throw new ApiError(resp.errorMessage || "Erro na simulacao");
  return resp.state;
}

export async function simInit(program: number[]): Promise<Mic1CpuState> {
  return callSim("/sim/init", { program });
}

export async function simStepMicro(state: Mic1CpuState): Promise<Mic1CpuState> {
  return callSim("/sim/step-micro", { state });
}

export async function simStepMacro(state: Mic1CpuState): Promise<Mic1CpuState> {
  return callSim("/sim/step-macro", { state });
}

export { ApiError };

export async function simStepSubcycle(state: Mic1CpuState): Promise<Mic1CpuState> {
  return callSim("/sim/step-subcycle", { state });
}
