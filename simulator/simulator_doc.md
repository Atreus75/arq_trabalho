# Documentação do Simulador

O simulador implementa a emulação do ciclo de instrução da arquitetura MIC-1 sobre a ISA MAC-1.

---

## 1. Constantes e Configurações

### `#define MEMO_SIZE 4096`
Capacidade da memória principal em palavras de 16 bits. Define o espaço de endereçamento de `0x000` a `0xFFF`.

---

## 2. Estruturas de Dados

### `typedef struct componentesMIC1 MIC1`
Representa o estado dos registradores e da memória da máquina virtual:

| Campo | Tipo | Descrição |
| :--- | :--- | :--- |
| `PC` | `uint16_t` | Contador de programa (aponta para a próxima instrução a ser buscada). |
| `AC` | `int16_t` | Acumulador de 16 bits com sinal. |
| `SP` | `uint16_t` | Apontador de pilha (Stack Pointer). |
| `memory` | `uint16_t[MEMO_SIZE]` | Vetor contíguo de 4096 palavras representando a memória principal. |
| `running` | `int` | Flag de execução: `1` indica máquina ativa, `0` indica parada (`halt`). |

---

## 3. Funções

### `void mic1_init(MIC1 *m)`
Inicializa todos os registradores e a memória com valor zero.
* **Parâmetros:**
  * `MIC1 *m`: Ponteiro para a estrutura de máquina a ser inicializada.
* **Comportamento:**
  * Define `PC = 0`, `AC = 0`, `SP = 0`, `running = 0`.
  * Preenche todo o vetor `memory[0..4095]` com `0`.

### `void mic1_step(MIC1 *m)`
Executa um ciclo único de instrução (busca, decodificação e execução).
* **Parâmetros:**
  * `MIC1 *m`: Ponteiro para a estrutura contendo o estado da máquina.
* **Fluxo de Operação:**
  1. **Busca:** Carrega `instruction = m->memory[m->PC]` e incrementa `m->PC++`.
  2. **Decodificação:**
     * `opcode = (instruction >> 12) & 0xF` (bits 15 a 12).
     * `operando = instruction & 0x0FFF` (bits 11 a 0).
  3. **Execução:** Desvia para o bloco correspondente ao opcode via `switch-case`.
  4. Para instruções não implementadas ou `halt`, define `m->running = 0`.

