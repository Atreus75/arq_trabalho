/**
 * Camada de API: unico lugar do front que sabe fazer `fetch`. O resto
 * da aplicacao (store, componentes) so conhece estas funcoes, nunca a
 * URL ou o formato de rede diretamente -- isso permite trocar o
 * transporte (ex: WebSocket no futuro) sem tocar em UI ou estado.
 */
import type { AssembleResult, Mic1CpuState } from "../types/mic1";

// Em desenvolvimento, o Vite roda em outra porta e usa proxy (ver
// vite.config.ts) para "/api" -> backend. Em producao (build servido
// pelo proprio servidor C), "/api" ja e relativo ao mesmo host.
const API_BASE = "/api";

class ApiError extends Error {
  status?: number;
  constructor(message: string, status?: number) {
    super(message);
    this.name = "ApiError";
    this.status = status;
  }
}

async function postJson<T>(path: string, body: unknown): Promise<T> {
  let response: Response;
  try {
    response = await fetch(`${API_BASE}${path}`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    });
  } catch (networkErr) {
    throw new ApiError(
      `Nao foi possivel conectar ao back-end em ${API_BASE}${path}. Verifique se o simulador esta aberto. (${
        networkErr instanceof Error ? networkErr.message : String(networkErr)
      })`
    );
  }

  let json: unknown;
  try {
    json = await response.json();
  } catch {
    throw new ApiError(`O back-end respondeu com um corpo que nao e JSON valido (status ${response.status}).`, response.status);
  }

  if (!response.ok) {
    const message =
      json && typeof json === "object" && "errorMessage" in json
        ? String((json as { errorMessage: unknown }).errorMessage)
        : `Erro HTTP ${response.status}`;
    throw new ApiError(message, response.status);
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

interface SimOkResponse {
  ok: true;
  state: Mic1CpuState;
}
interface SimErrResponse {
  ok: false;
  errorMessage: string;
}
type SimResponse = SimOkResponse | SimErrResponse;

async function callSim(path: string, body: unknown): Promise<Mic1CpuState> {
  const resp = await postJson<SimResponse>(path, body);
  if (!resp.ok) throw new ApiError(resp.errorMessage);
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
