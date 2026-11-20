# Architecture Cache
Last updated: 2026-11-19 — Added execution flow diagrams (`docs/fluxo-execucao.md`, `docs/fluxo-execucao.html`) and removed `.agents/architecture_cache.md` from `.gitignore`

## Stack
- **Languages & Versions**:
  - Backend / Simulator Engine: C (`-std=c11`, compiled with `cc` / `gcc`, `-Wall -Wextra -Wpedantic -O2`) `[Observed: Makefile, compilar.sh, compilar.bat]`
  - Frontend: TypeScript (`~6.0.2`), React (`^19.2.8`), React DOM (`^19.2.8`) `[Observed: frontend/package.json]`
- **Runtime & Libraries**:
  - C Backend: POSIX sockets + `pthread` + `libm` on Linux (`winsock2` + `shell32` on Windows) `[Observed: simulator/src/server.c, Makefile, compilar.bat]`
  - Frontend State & Styling: Zustand (`^5.0.15`), Tailwind CSS (`^3.4.19`), PostCSS (`^8.5.26`), Autoprefixer (`^10.5.4`) `[Observed: frontend/package.json]`
- **Package Manager & Lock File**:
  - `npm` with `frontend/package-lock.json` `[Observed: frontend/package-lock.json]`
- **Build Toolchain & Commands**:
  - C Simulator Build: `make` (or `bash compilar.sh` on Linux, `compilar.bat` on Windows) -> compiles `bin/mic1-local` and copies to `bin/mic1-linux-x64` `[Observed: Makefile, compilar.sh]`
  - Frontend Production Build: `cd frontend && npm run build` (`tsc -b && vite build`, outputs static bundle to `../web`) `[Observed: frontend/package.json, frontend/vite.config.ts]`
  - Frontend Linter: `cd frontend && npm run lint` (`oxlint`) `[Observed: frontend/package.json]`
  - Application Launcher: `./iniciar.sh` (Linux) or `iniciar.bat` (Windows) `[Observed: iniciar.sh, iniciar.bat]`

## Conventions
- **Code Style & Language Idioms**:
  - C code compiles strictly under `-std=c11 -Wall -Wextra -Wpedantic -O2` with zero warnings `[Verified]`.
  - Frontend uses strict TypeScript (`tsc -b`), functional React components, and Win98 aesthetic components (`frontend/src/components/win98/`).
  - Variables and internal struct/store fields are localized to idiomatic Brazilian Portuguese (`ulaA`, `saidaUla`, `saidaDeslocador`, `numVias`, `numConjuntos`, `viaVitima`, `codigoFonte`, `estadoCpu`, `enderecosGravados`), while preserving `flag`/`flags`, `length`/`hasLength`, hardware acronyms (`pc`, `ac`, `sp`, `ir`, `tir`, `mar`, `mbr`, `mpc`, `mir`), and JSON wire protocol keys `[Scope: Permanent]`.
- **Architectural & Design Patterns**:
  - **Microprogrammed Execution Only**: The C engine (`simulator/src/mic1.c`) executes instructions exclusively through the 79-word control memory ROM (`simulator/src/microcode.c`) across 4 subcycles per microinstruction cycle. Macroinstructions are never executed directly in C `[Observed: simulator/src/mic1.c]`.
  - **Stateless REST/JSON Protocol**: HTTP endpoints (`/api/*`) receive and return the full serialized CPU and cache state (`Mic1State` + `Mic1Cache`) on each step `[Observed: simulator/src/protocol.c]`.
  - **23-Instruction MAC-1 ISA & Termination Idiom**: The ISA has 23 instructions (`LODD`..`DESP`). `CALL` uses a 6-bit prefix (`111000`) and 10-bit address (`0..1023`). Programs terminate via the canonical loop idiom `END: JUMP END` (no proprietary `HALT` opcode) `[Observed: simulator/src/isa.c, exemplos/*.asm]`.
  - **Assembler Symbol Separation**: Two-pass in-memory assembler (`simulator/src/assembler.c`) maintains strictly separate symbol tables for code labels (`flags`, declared with `:`) and implicit variables (`vars`, allocated after program instructions). Jump/call instructions resolve symbols exclusively in `flags` (or explicit numeric addresses), while data instructions resolve/allocate identifiers in `vars` `[Observed: simulator/src/assembler.c]`.
  - **Datapath & Shifter**: Separate `mbrLeitura` and `mbrEscrita` registers (`mbrLeitura` captured in Subcycle 4 on 2nd `RD` cycle and fed to `AMUX` in Subcycle 2). Right shift (`SH=1`) is a true 1-bit arithmetic right shift preserving bit 15 (`(saidaUla >> 1) | (saidaUla & 0x8000)`) `[Observed: simulator/src/mic1.c]`.
- **Explicit Anti-Patterns**:
  - Never reintroduce `OP_HALT` (`0xFF00`), the `WORD` directive, or disk writes to `programas/` during assembly.
  - Never replicate the right-shift arraycopy bug from `inspiration/SimuladorVinod/src/modelo/Deslocador.java`.
  - Never leave `bin/mic1-linux-x64` or `web/` out of sync after modifying C or Frontend sources.

## Module Map
- `simulator/src/` — C11 Simulator & HTTP Server:
  - `simulator/src/main.c` — CLI entry point (`--port`, `--no-browser`, `--json ROTA`) `[Entry handler]`
  - `simulator/src/server.c` & `server.h` — Loopback multithreaded HTTP/1.1 server serving `web/` and `/api/*` `[Entry handler]`
  - `simulator/src/protocol.c` & `protocol.h` — JSON serialization/deserialization and route dispatcher `[Entry handler / Business logic]`
  - `simulator/src/mic1.c` & `mic1.h` — MIC-1 4-subcycle datapath engine, registers (`PC`, `AC`, `SP`, `IR`, `TIR`, `MAR`, `mbrRead`, `mbrWrite`, `A..F`), ALU, shifter `[Business logic / Model]`
  - `simulator/src/microcode.c` — 79-word (`0..78`) 32-bit control memory ROM and RTL text descriptions `[Model]`
  - `simulator/src/isa.c` — 23-instruction MAC-1 table, `mic1_encode`, `mic1_decode` `[Business logic / Model]`
  - `simulator/src/assembler.c` & `assembler.h` — Two-pass in-memory assembler with `flags` and `vars` tables `[Business logic]`
  - `simulator/src/cache.c` & `cache.h` — Configurable cache memory subsystem (Direct, Fully Associative, Set-Associative; LRU/FIFO; Write-Through/Write-Back) `[Business logic / Model]`
  - `simulator/src/json.c` & `json.h` — Zero-dependency JSON parser and writer `[Utility]`
- `frontend/src/` — React + TypeScript + Tailwind UI:
  - `frontend/src/main.tsx` & `App.tsx` — Root 3-window Win98 desktop layout `[Presentation]`
  - `frontend/src/api/client.ts` — HTTP fetch client for `/api/*` `[Data access]`
  - `frontend/src/store/mic1Store.ts` — Zustand store managing editor, CPU, cache, execution timer, and single-shot PC breakpoint `[Business logic / State]`
  - `frontend/src/store/examples.ts` — Built-in assembly examples (`Soma simples`, `Função e pilha`, `Array da aula`, `Fibonacci da referência`) `[Model]`
  - `frontend/src/types/mic1.ts` & `rtlEffects.ts` — TypeScript domain contracts, instruction formatting, and RTL templates `[Model / Utility]`
  - `frontend/src/components/EditorPanel.tsx` — Window 1: Assembly editor, `Montar`, `Gravar código na MP`, `Montar e iniciar`, symbol table, compiled binary preview `[Presentation]`
  - `frontend/src/components/ProcessorPanel.tsx` — Window 2: Cycle counter, execution time (`ms`), 11 registers table (`IR`/`TIR` in 16-bit binary), bit panel, execution controls `[Presentation]`
  - `frontend/src/components/ExecutionControls.tsx` — Step unit selector (`cycle` default), delay in `ms`, single-shot PC breakpoint with `Aplicar` button `[Presentation]`
  - `frontend/src/components/InternalPanel.tsx` — Collapsible datapath inspector (MIR fields, subcycles, latches, `MBR (Read)` / `MBR (Write)`, 79-word control memory) `[Presentation]`
  - `frontend/src/components/MemoryPanel.tsx` — Window 3: Main Memory tab (with continuous stack highlight `addr >= SP` in blue and `PC` in gray) and Cache tab (configuration, Hit/Miss dashboard, cache lines table) `[Presentation]`
- `exemplos/` — Canonical MAC-1 assembly programs (`soma.asm`, `fibonacci.asm`, `funcao-pilha.asm`, `array.asm`) `[Model]`
- `docs/` — Project documentation (`microprograma.txt`, `LICENCAS-DA-INTERFACE.txt`, `fluxo-execucao.md`, `fluxo-execucao.html`) `[Documentation]`
- `inspiration/` — Read-only Java/Swing reference project (`SimuladorVinod`) `[Reference]`

## Entry Points
- **CLI Binary (`bin/mic1-local` / `bin/mic1-linux-x64`)** `[Observed: simulator/src/main.c]`:
  - `./bin/mic1-local [--port 1024..65535] [--no-browser]`: starts local HTTP server on `127.0.0.1:3001` (default). Auth: loopback `Host`/`Origin` check only.
  - `./bin/mic1-local --json <ROTA>`: reads JSON payload from `stdin`, invokes `protocol_request`, prints JSON response to `stdout`.
- **HTTP Endpoints (`127.0.0.1:3001`)** `[Observed: simulator/src/server.c, simulator/src/protocol.c]`:
  - `GET /api/health` -> returns engine health status JSON.
  - `GET /api/microcode` -> returns the 79 control memory words (`address`, `word`, `text`).
  - `POST /api/assemble` -> body `{ sourceCode: string }`, returns `{ success, program, labelTable, errors }`.
  - `POST /api/sim/init` -> body `{ program: number[], cache?: Mic1CacheState }`, returns `{ ok: true, state, cache }`.
  - `POST /api/sim/step-subcycle` -> body `{ state: Mic1CpuState, cache?: Mic1CacheState }`, advances 1 subcycle.
  - `POST /api/sim/step-micro` -> body `{ state: Mic1CpuState, cache?: Mic1CacheState }`, advances 1 microinstruction cycle (4 subcycles).
  - `POST /api/sim/step-macro` -> body `{ state: Mic1CpuState, cache?: Mic1CacheState }`, advances 1 macroinstruction (up to `mpc == 0, nextSubcycle == 1`).
  - `POST /api/sim/run` -> body `{ state: Mic1CpuState, maxCycles?: number }`, advances multiple macroinstructions.
  - `POST /api/sim/cache-config` -> body `{ state?: Mic1CpuState, config: Partial<Mic1CacheConfig> }`, reconfigures cache geometry/policies and flushes dirty lines.

## Dependency Graph
- `main.c` -> `server.h`, `protocol.h`
- `server.c` -> `protocol.h`, `json.h`
- `protocol.c` -> `mic1.h`, `assembler.h`, `cache.h`, `json.h`
- `mic1.c` -> `mic1.h` (`cache.h`)
- `assembler.c` -> `assembler.h`, `mic1.h`, `json.h`
- `isa.c`, `microcode.c` -> `mic1.h`
- `frontend/src/App.tsx` -> `EditorPanel.tsx`, `ProcessorPanel.tsx`, `MemoryPanel.tsx` -> `useMic1Store` (`mic1Store.ts`) -> `api/client.ts` -> `/api/*`

## Data Layer
- In-memory state only (`Mic1State` with `uint16_t memory[4096]`, `uint16_t r[16]`, and `Mic1Cache` with up to 64 cache lines). No external database or disk persistence `[Observed: simulator/src/mic1.h, simulator/src/cache.h]`.

## Testing Infrastructure
- `[No test runner configured]` — Legacy test suite (`tests/`, `testar.sh`, `testar.bat`, `--selftest`) was intentionally removed per user directive (`R4` / Checklist #6) to match the reference repository structure. Verification is performed via `make`, `bash compilar.sh`, `cd frontend && npm run build`, and CLI `--json` / UI execution of `exemplos/*.asm`.

## Documentation
- `README.md` (root), `frontend/README.md`, `docs/microprograma.txt` (79-word control memory reference), `docs/LICENCAS-DA-INTERFACE.txt`, `docs/fluxo-execucao.md` (Mermaid architecture & sequence diagrams), `docs/fluxo-execucao.html` (interactive execution flow presentation for video recording).
- Last drift baseline: 2026-11-19 — 0 gaps found (`docs/microprograma.txt`, `docs/fluxo-execucao.md`, and `docs/fluxo-execucao.html` synchronized with codebase).

## Open Issues
- Automated test runner intentionally omitted per user directive (`R4` / `F-17`).

## Pending Edge Cases
<!-- Empty at initialization -->

## Pending Manual Testing Steps
- [MT-001] Montador & Término (`F-01`, `F-11`, `F-12`, `F-13`) — Verificar botões separados "Montar" / "Gravar código na MP", variáveis implícitas e rejeição de `HALT`/`WORD`
  Source: Parity alignment session, 2026-11-10
  Steps:
    1. Abrir `1 - Editor.asm`, selecionar "Fibonacci da referência" e clicar em `🔨 Montar`.
    2. Marcar "Ver tabela de símbolos" e verificar os endereços de `START`, `LOOP`, `END`, `var1` (17) e `var2` (18), e que o programa compilado termina no endereço 16 (`END: JUMP END`) sem linhas `WORD 0` extras nos endereços 17/18.
    3. Clicar em `💾 Gravar código na MP` e verificar que a memória recebe as instruções sem resetar configurações existentes.
    4. Digitar `HALT` ou `WORD 10` no editor e clicar em `🔨 Montar`.
  Expected: Montagem de Fibonacci gera 17 palavras (0..16); `HALT` e `WORD` produzem erro de "Mnemonico desconhecido".
  Related: `simulator/src/assembler.c`, `frontend/src/components/EditorPanel.tsx`
  Status: Pending

- [MT-002] Instrução `CALL` de 10 bits (`F-03`) e `DESP` em 13 ciclos (`F-02`) — Verificar codificação e contagem de ciclos
  Source: Parity alignment session, 2026-11-10
  Steps:
    1. No `1 - Editor.asm`, selecionar "Função e pilha" e clicar em `🔨 Montar`. Verificar o binário de `CALL DOBRO` (prefixo `111000`).
    2. Testar `CALL 1024` no editor e clicar em `🔨 Montar` (deve rejeitar com erro `CALL nao suporta endereco >= 1024`).
    3. Testar `DESP 1` seguido de `END: JUMP END`, clicar em `▶ Montar e iniciar`, mudar "Modo do passo" para `Instrução` e clicar em `→ Passo (Instrução)`.
  Expected: `CALL` aceita apenas `0..1023` com prefixo `111000`; `DESP 1` executa em exatos 13 ciclos (52 subciclos).
  Related: `simulator/src/isa.c`, `simulator/src/microcode.c`
  Status: Pending

- [MT-003] Painel do Processador, 11 Registradores, Controles e Breakpoint (`F-07`, `F-08`, `F-09`, `F-16`) — Verificar UI e execução contínua
  Source: Parity alignment session, 2026-11-10
  Steps:
    1. Na janela `2 - Processador`, verificar que `Ciclos` e `Tempo de execução (ms)` estão no topo em destaque e que a tabela exibe os 11 registradores (`PC`, `AC`, `SP`, `IR`, `TIR`, `A..F`) com `IR` e `TIR` em binário de 16 bits.
    2. Abrir `Ver funcionamento interno` e verificar a presença de `MBR (Read)`, `MBR (Write)` e `Memória de controle (79/79 palavras)`.
    3. Com "Fibonacci da referência" carregado, digitar `16` em `Pausar quando PC`, clicar em `Aplicar`, reduzir a pausa para `50 ms`, mudar o passo para `Instrução` e clicar em `▶ Executar`.
    4. Após pausar automaticamente em `PC = 16`, clicar novamente em `▶ Executar` e verificar que a simulação roda em `END: JUMP END` sem travar no breakpoint nem abortar com erro de 10.000 instruções.
  Expected: Pausa automática única em `PC = 16` com `1597` no topo da pilha (`m[4078]`), e execução contínua livre após retomada.
  Related: `frontend/src/components/ProcessorPanel.tsx`, `frontend/src/components/ExecutionControls.tsx`, `frontend/src/store/mic1Store.ts`
  Status: Pending

- [MT-004] Painel de Memória Principal e Aba Cache (`R3`, `F-16`) — Verificar destaque da pilha e políticas de Cache
  Source: Parity alignment session, 2026-11-10
  Steps:
    1. Na janela `3 - Memória` (aba `Memória Principal`), clicar em `Pilha (SP)` após rodar Fibonacci e verificar que todos os endereços `>= SP` ficam destacados em azul (`#0000cc`) e o endereço de `PC` em cinza.
    2. Clicar na aba `Cache ●` no topo da janela `3 - Memória`.
    3. Executar passos do programa e verificar que a barra de `Hit Rate`, os contadores de `Leituras`/`Escritas`, o indicador `[HIT]`/`[MISS]` do último acesso e a tabela de linhas/conjuntos/tags atualizam em tempo real.
    4. Alterar o mapeamento para `Associativo por Conjunto` (2 ou 4 vias) ou `Totalmente Associativo`, trocar para `FIFO` e `Write-Back`, clicar em `Aplicar Configuração` e dar novos passos.
  Expected: Pilha inteira destacada em azul; cache registra hits/misses, preenche palavras nos blocos, marca `Sujo = 1` em `Write-Back` e atualiza a geometria de conjuntos/vias.
  Related: `simulator/src/cache.c`, `frontend/src/components/MemoryPanel.tsx`
  Status: Pending

## Release Baseline
- Initial Git baseline initialized on 2026-11-10 (`72aa065` — `.gitignore` exclusion commit).
