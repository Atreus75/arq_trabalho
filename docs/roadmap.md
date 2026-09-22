# Roteiro de Implementação e Lista Granular de Tarefas

Este documento define o planejamento em fases sequenciais de engenharia para a construção completa do simulador MIC-1, com objetivos e critérios de aceitação para cada tarefa técnica.

---

## 1. Visão Geral das Fases

```
Fase 1: Montador CLI & Definição de Opcodes
   └── Parsing do código Assembly, geração do binário e resolução de rótulos (labels).
Fase 2: Motor do Simulador & Emissão de Estado
   └── Virtualização de memória, ciclo de busca/decodificação/execução e saída JSON.
Fase 3: Camada de Middleware (Node.js)
   └── Gerenciamento de subprocessos, streaming via IPC e endpoints REST/WebSocket.
Fase 4: Interface Gráfica Web (React)
   └── Layout em 3 janelas, editor de código, painel de controle e inspeção de registradores/memória.
Fase 5: Conteinerização, Automação & Validação
   └── Dockerfile multi-stage, suíte de testes de ponta a ponta e tratamento de casos de borda.
```

---

## 2. Tarefas por Fase

### Fase 1: Montador CLI & Definição de Opcodes (C)
**Marco de Entrega:** Executável de linha de comando `bin/mic1-as` que recebe um arquivo com código Assembly e gera o binário correspondente ou mensagens estruturadas de erro sintático.

| ID | Descrição da Tarefa | Entregável / Critério de Aceitação |
| :--- | :--- | :--- |
| **T1.1** | **Especificação da ISA e Tabela de Opcodes** | Definição da enumeração de opcodes, formato da palavra (16 bits) e tabela de instruções em `assembler.h`. |
| **T1.2** | **Analisador Léxico (Lexer & Tokenizer)** | Parser capaz de extrair mnemônicos, operandos e identificadores de rótulos, ignorando espaços em branco e comentários. |
| **T1.3** | **Tabela de Símbolos & Resolução em Duas Etapas** | Passo 1: mapear endereços de rótulos definidos; Passo 2: resolver referências e saltos à frente/atrás. |
| **T1.4** | **Suporte a Parâmetros de Linha de Comando** | Suporte às flags `-o <arquivo_saida>` e `-i` (modo interativo com emissão de erros estruturados com linha e coluna). |
| **T1.5** | **Bateria de Testes do Montador** | Conjunto de 10 programas Assembly cobrindo laços, manipulação de pilha e casos de borda para validação do binário gerado. |

---

### Fase 2: Motor do Simulador & Emissão de Estado (C)
**Marco de Entrega:** Executável independente `bin/mic1-sim` que carrega binários montados, simula as transições de ciclo e serializa o estado em `stdout`.

| ID | Descrição da Tarefa | Entregável / Critério de Aceitação |
| :--- | :--- | :--- |
| **T2.1** | **Inicialização de Memória e Registradores** | Rotina de reset para zerar registradores/memória e carregar blocos de código a partir do endereço base `0x000`. |
| **T2.2** | **Ciclo de Busca, Decodificação e Execução** | Despachante de instruções decodificando o opcode de 4 bits e efetuando as transferências entre registradores. |
| **T2.3** | **Operações da ULA e Atualização de Flags** | Operações aritméticas no acumulador (soma, subtração, lógica) e cálculo correto das flags Z (zero) e N (negativo). |
| **T2.4** | **Subsistema de Gerenciamento da Pilha** | Operações `PUSH` e `POP` com verificação estrita de limites do ponteiro de pilha para evitar overflow/underflow. |
| **T2.5** | **Emissor de Estado em JSON (Modo Passo a Passo)** | Flag `--step` no executável emitindo o estado de registradores e deltas de memória como linhas JSON únicas no `stdout`. |

---

### Fase 3: Camada de Middleware (Node.js)
**Marco de Entrega:** Serviço em Express/Fastify que expõe as capacidades dos binários em C por meio de rotas HTTP e WebSockets.

| ID | Descrição da Tarefa | Entregável / Critério de Aceitação |
| :--- | :--- | :--- |
| **T3.1** | **Invólucro de Subprocessos (IPC Wrapper)** | Execução assíncrona de `mic1-as` e `mic1-sim` usando `child_process.spawn`, tratando pipes sem travamentos. |
| **T3.2** | **Endpoint de Compilação (`POST /api/assemble`)** | Recebe texto Assembly, executa o montador e retorna o binário/hexadecimal ou a lista de erros de sintaxe. |
| **T3.3** | **Gerenciador de Sessão de Simulação** | Canal de comunicação (WebSocket ou streaming) para envio de comandos de controle (`STEP`, `RUN`, `RESET`). |
| **T3.4** | **Sanitização de Entrada e Limites de Execução** | Validação de tamanho de payloads, prevenção contra injeção de comandos de shell e timeout para laços infinitos. |

---

### Fase 4: Interface Gráfica Web (React)
**Marco de Entrega:** Aplicação web interativa no navegador com layout em três janelas conectada à API Node.js.

| ID | Descrição da Tarefa | Entregável / Critério de Aceitação |
| :--- | :--- | :--- |
| **T4.1** | **Setup do Projeto e Layout Base** | Inicialização com React e Vite contendo estrutura de três colunas ou painéis redimensionáveis. |
| **T4.2** | **Janela 1: Editor de Código Assembly** | Área de texto com numeração de linhas, realce de erros retornados pelo montador e visualizador lateral do binário. |
| **T4.3** | **Janela 2: Barra de Controle de Execução** | Botões de ação (`Montar`, `Executar Tudo`, `Avançar Passo`, `Resetar`) e controle deslizante de velocidade do clock. |
| **T4.4** | **Janela 3A: Painel de Registradores** | Visualização em tempo real de PC, AC, SP, IR e flags Z/N, com realce visual nos valores modificados no passo atual. |
| **T4.5** | **Janela 3B: Painel de Memória** | Grade ou lista virtualizada da memória principal (`0x000` a `MEM_SIZE`) com indicadores visuais para PC e SP. |
| **T4.6** | **Integração com a API e Gerenciamento de Estado** | Conexão do cliente HTTP/WS aos componentes visuais, aplicando os deltas de memória recebidos a cada passo. |

---

### Fase 5: Conteinerização, Automação & Validação
**Marco de Entrega:** Imagem de contêiner e suíte automatizada de testes cobrindo todo o ciclo do sistema.

| ID | Descrição da Tarefa | Entregável / Critério de Aceitação |
| :--- | :--- | :--- |
| **T5.1** | **Makefile Unificado** | Regras de compilação dos alvos em C com flags de compilação estritas (`-Wall -Wextra -Werror -pedantic`). |
| **T5.2** | **Dockerfile Multi-Stage (Alpine)** | Estágio 1: compilação do C (gcc) e build do React. Estágio 2: imagem leve com runtime Node.js e estáticos. |
| **T5.3** | **Suíte de Testes Ponta a Ponta** | Script automatizado que envia Assembly de teste, simula passos e valida os valores finais de registradores e memória. |
| **T5.4** | **Tratamento de Casos de Borda** | Validação de divisão por zero, acesso a memória fora dos limites, overflow de pilha e interrupção graciosa de execução. |

