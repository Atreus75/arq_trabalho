# Simulador da Arquitetura MIC-1

Este repositório contém o projeto de um simulador educacional da arquitetura de computador **MIC-1** com interface web interativa. O sistema realiza a montagem de código Assembly para binário, simula o caminho de dados da CPU e exibe o estado de registradores e memória passo a passo.

---

## 1. Resumo Executivo

O objetivo do projeto é fornecer uma aplicação web composta por três visualizações principais:

1. **Janela 1 (Editor & Binário):** Editor de texto para código Assembly MIC-1 acompanhado da visualização simultânea do código binário/hexadecimal compilado.
2. **Janela 2 (Controles de Execução):** Painel interativo com botões de controle de fluxo de execução (`Montar`, `Executar Tudo`, `Avançar Passo`, `Resetar` e controle de velocidade/clock).
3. **Janela 3 (Estado do Sistema):** Inspeção em tempo real dos registradores internos do processador (PC, AC, SP, IR, etc., e flags Z/N) e da memória principal.

---

## 2. Pontos Críticos em Aberto (Dúvidas para o Professor / Monitor)

> [!CAUTION]
> **Decisões Bloqueantes:** Os seguintes itens dependem de alinhamento com o professor Vinod ou com os monitores para congelar a especificação do motor em C.

1. **Variante da ISA (Conjunto de Instruções):**
   - *Dúvida:* O MIC-1 clássico de Tanenbaum simula microinstruções do IJVM, enquanto implementações didáticas comuns adotam o Assembly Mac-1 (ex.: `LODD`, `STOD`, `ADDD`, `SUBD`, `JPOS`, `JZER`, `JUMP`, `LOCO`, `PUSH`, `POP`).
   - *Impacto:* Define a tabela de opcodes do montador, tamanho da palavra de instrução (16 bits vs. 32 bits) e modos de endereçamento.
   - *Premissa Adotada Inicialmente:* Formato de instrução Mac-1 de 16 bits (4 bits de opcode, 12 bits de endereço/imediato).
1. **Definição da ISA — Confirmada como MAC-1:**
   - *Status:* Confirmada pela documentação raiz do repositório (`README.md`) e pelo código do montador (`simulator/assembler.c`).
   - *Formato:* Instruções de 16 bits (4 bits de opcode, 12 bits de endereço/imediato). O montador já implementa mnemônicos como `addd`, `subd`, `lodd`, `stod`, etc.

2. **Granularidade do Passo a Passo (Macro-instrução vs. Micro-instrução):**
   - *Dúvida:* O avanço "passo a passo" deve ser por **instrução Assembly** (macro-passo) ou por **ciclo de microcódigo do caminho de dados** (micro-passo via control store, MPC e MIR)?
   - *Impacto:* O micro-passo exige modelar a memória de controle interna (ROM de microprograma). O macro-passo modela apenas a transição de estado no nível da instrução da ISA.

3. **Formato e Espaço de Memória:**
   - *Dúvida:* A memória é endereçada por palavra (16 bits) ou por byte (8 bits), e qual é a capacidade total suportada (ex.: 4096 palavras / espaço de 12 bits)?
   - *Impacto:* Define o tamanho do buffer em `simulator.h` e o cálculo de deslocamento do Stack Pointer (SP).

---

## 3. Navegação da Documentação

- [Arquitetura e Estruturas de Dados](arquitetura.md): Diagrama de fluxo, divisão de camadas e definição dos `structs` em C.
- [Roteiro e Tarefas (Roadmap)](roadmap.md): Fases de implementação e lista detalhada de entregáveis sem atribuição de pessoas.

