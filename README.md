# MIC-1.sim — versão microprogramada em C

Simulador didático da MIC-1 de 16 bits usada para executar MAC-1.
O motor, o montador e o servidor local são escritos em **C**. A interface
React mantém o visual de Windows clássico e o cavalinho discreto.

## Abrir no Windows 10/11 de 64 bits

1. Extraia **o ZIP inteiro** para uma pasta, por exemplo `Documentos\mic1-simulator`.
2. Abra **iniciar.bat**. O navegador abre no endereço `http://127.0.0.1:3001`.
3. Mantenha a janela do simulador aberta. Fechá-la encerra o servidor.

A versão pronta inclui o executável e a interface compilada. **Não exige Java,
Node.js, npm, GCC ou internet para usar.** Não execute de dentro da visualização
do ZIP. Não abra `web/index.html` diretamente, pois a interface conversa com o
motor C pelo servidor local. O programa atende somente no próprio computador.

Se o navegador não abrir automaticamente, copie o endereço acima. Se a porta
3001 estiver ocupada, feche outra instância ou execute `iniciar.bat --port 3002`.
O endereço, nesse caso, termina em `:3002`.

Em Linux x86-64: `sh iniciar.sh`, depois abra o endereço mostrado no terminal.
O código também pode ser recompilado; veja abaixo. Não há binário pronto de macOS.

## Usar e apresentar

1. Escolha **Soma simples**, **Função e pilha**, **Array da aula** ou **Fibonacci da referência**.
2. Clique em **Montar e iniciar**. Aparecem as palavras binárias e o estado inicial.
3. Use **Um passo** para avançar ou **Executar** para continuar automaticamente.
4. Em **Cada passo**, escolha:
   - **Instrução:** termina uma instrução MAC-1, incluindo sua busca.
   - **Ciclo:** termina a microinstrução em andamento (quatro subciclos ao todo).
   - **Subciclo:** avança apenas uma das quatro etapas internas.
5. Abra **Ver funcionamento interno** para observar os registradores, os sinais
   de controle, a ULA, o deslocador, os pedidos de memória e o histórico do passo.
6. **Pausar no endereço** interrompe antes da instrução naquele endereço de memória.
   Campo vazio desliga a pausa automática. Continuar permite executar a instrução
   em que parou. O breakpoint é verificado nas fronteiras entre instruções.
7. **Reiniciar** restaura o programa montado, a memória, os registradores e os contadores.

Alternar o tamanho do passo preserva a execução parcial. Por exemplo, após um
subciclo, escolher Ciclo executa os três restantes. Escolher Instrução completa
a instrução atual. Pausar deixa terminar apenas um pedido de passo já enviado.

O painel de memória mostra **Gravados**, **Programa**, **Pilha** ou uma região
escolhida. Escritas de zero também aparecem. O destaque do programa acompanha a
instrução em execução; o PC pode ter sido incrementado durante a busca.

## Como funciona, em linguagem direta

O assembly é o programa que você escreve: `LOCO`, `ADDD`, `CALL` etc. O montador
em C transforma cada instrução em uma palavra de 16 bits e resolve os nomes de
rótulos e variáveis. O texto também é salvo em `programas/`.

A memória principal armazena essas palavras e os dados. A CPU possui outra memória,
chamada **memória de controle**, que contém o microprograma. Esse microprograma
explica à MIC-1 como executar as instruções MAC-1. O motor lê os campos da palavra
de controle; não usa um `switch` de opcodes para calcular diretamente o efeito
final das instruções. Há um modelo separado com esses efeitos somente nos testes,
para comparar o resultado esperado com o microprograma.

**Uma instrução MAC-1 usa várias microinstruções. Cada microinstrução ocupa um ciclo,
e cada ciclo tem quatro subciclos:**

| Subciclo | O que acontece |
|---|---|
| 1 | O MPC indica uma posição da memória de controle, e sua palavra é carregada no MIR. |
| 2 | O dado de uma leitura concluída é capturado no MBR; os registradores alimentam os latches A e B, e o AMUX escolhe a entrada A da ULA. |
| 3 | A ULA faz a operação, atualiza N/Z e entrega seu resultado ao deslocador. |
| 4 | Os destinos habilitados recebem os valores; a memória avança seu pedido e o controle escolhe a próxima microinstrução. |

O MBR captura uma leitura na passagem do primeiro para o segundo subciclo. Um
pedido de leitura/escrita exige o sinal RD/WR em dois ciclos consecutivos. O
primeiro inicia o acesso; o segundo o conclui. Uma leitura concluída fica
pendente de captura no MBR no subciclo 2 seguinte. Isso impede que o motor
trate uma leitura como instantânea.

### Exemplo: LOCO 7

`LOCO 7` vira a palavra `0x7007`. A MIC-1 busca essa palavra na memória, coloca-a
no IR e percorre o caminho do microprograma que identifica LOCO. A ULA faz o AND
entre IR e AMASK (`0x0FFF`), deixando apenas o operando 7, que é gravado em AC.
O controle volta ao início da busca. Neste microprograma, essa instrução ocupa
**7 ciclos = 28 subciclos**. O contador de instruções só aumenta ao terminá-la.

Na soma do primeiro exemplo: `LOCO 7` coloca 7 no AC; `STOD 100` guarda o valor;
`LOCO 5` coloca 5 no AC; `ADDD 100` lê 7 da memória e soma, deixando 12 no AC;
`STOD 100` salva 12; `HALT` encerra. A tela simples acompanha o resultado, e o
painel interno mostra as operações menores que o produziram.

### Registradores principais

- **PC:** contador do programa. Tem 16 bits; o acesso físico à memória usa os 12 inferiores.
- **AC:** acumulador onde ficam os resultados das operações.
- **SP:** topo da pilha. Começa em 4096; o primeiro PUSH/CALL usa o endereço 4095.
- **IR/TIR:** instrução carregada e cópia temporária usada na decodificação.
- **MAR:** endereço físico da leitura/escrita; **MBR:** palavra transferida.
- **MPC:** endereço da microinstrução; **MIR:** palavra de controle carregada.
- **A–F:** registradores auxiliares. Também existem 0, 1, −1, AMASK e SMASK constantes.
- **N/Z:** indicam resultado negativo/zero na saída da ULA, **antes do deslocador**.

## Assembly aceito

As 23 instruções da tabela: LODD, STOD, ADDD, SUBD, JPOS, JZER, JUMP, LOCO,
LODL, STOL, ADDL, SUBL, JNEG, JNZE, CALL, PSHI, POPI, PUSH, POP, RETN, SWAP,
INSP e DESP. Há ainda a extensão HALT (`0xFF00`). JPOS inclui zero.

- Endereços/constantes x: 0 a 4095. INSP/DESP: operando y de 0 a 255.
- Uma instrução por linha; números decimais; espaços e linhas vazias são permitidos.
- Comentários começam com `;` ou `//`. Mnemônicos aceitam maiúsculas/minúsculas.
- Rótulos usam `nome:`; podem estar na mesma linha ou na linha anterior à instrução.
- Os nomes distinguem maiúsculas/minúsculas. Use letras sem acentos, dígitos e `_`,
  começando por letra ou `_`, com até 63 caracteres.
- `WORD -1` reserva uma palavra de dado. Faixa: -32768 a 65535.
- LODD/STOD/ADDD/SUBD também aceitam variáveis **implícitas**, como `STOD resultado`:
  se o nome não foi declarado, o montador reserva uma palavra zerada após o programa.
- Saltos e CALL exigem número ou rótulo declarado. LOCO, INSP/DESP e acessos locais
  também exigem um operando válido; não criam variáveis implicitamente.
- É possível colar palavras binárias de exatamente 16 bits, uma por linha.

Exemplo com variável implícita:

```asm
LOCO 7
STOD resultado
LOCO 5
ADDD resultado
STOD resultado
HALT
```

Código e dados compartilham a memória. Use HALT ou um salto para evitar executar
os dados como instruções. `JUMP` para o próprio endereço é um laço, não HALT.
Após 10.000 instruções na execução automática, a interface pausa; pode continuar.

## Relação com a referência Java

Referência examinada: https://github.com/csbassin/SimuladorVinod
Commit: `9547d113f9f67de9a6aaeb427cc7d95d9b6a4575` (22/09/2026).
O JAR enviado também foi inspecionado. O repositório tem alterações posteriores
à versão do JAR; a implementação C foi conferida contra a tabela MAC-1 e a lógica
do microprograma, sem simplesmente copiar cada particularidade do Java.

Preservados: caminho de dados de 16 bits, registradores e máscaras, memória
principal, palavras de controle de 32 bits, microprograma MAC-1, quatro subciclos,
montador com variáveis implícitas, controles de execução e pausa por endereço.

Detalhes definidos nesta versão:

- Memória principal: 4096 palavras de 16 bits; MAR usa 12 bits. Os registradores,
  incluindo PC, preservam 16 bits. PC=4096 acessa fisicamente m[0]; ao ultrapassar
  65535, retorna a 0. Desvios diretos usam os 12 bits do operando.
- ROM com 256 posições; 81 microinstruções usadas. O conteúdo é consultável pela
  interface e em `docs/microprograma.txt`; a edição da ROM é feita no código C.
- HALT é uma extensão explícita da tabela de 23 instruções. O microprograma o
  reconhece e chega a uma palavra com RD e WR simultâneos para encerrar.
- CALL mantém o operando de **12 bits** da tabela; não reproduz a restrição de
  10 bits presente no montador Java consultado. DESP usa FEyy e SMASK vale 00FF.
- Palavras reservadas fora das codificações aceitas geram diagnóstico e parada.
- Modelo de transferências entre registradores, não de sinais elétricos/portas
  lógicas. Não inclui cache, UART, interrupções, pipeline ou execução paralela.
  As 4096 posições são RAM, inclusive o topo usado pela pilha.
- A velocidade da animação é uma pausa entre passos; não representa a frequência
  física de um processador. O que mede a execução simulada são os contadores.

Referência complementar do formato de controle e das instruções:
https://docs.racket-lang.org/mic1/index.html . A UART daquela implementação não
faz parte do modelo desta entrega.

## Código e recompilação

| Arquivo/pasta | Responsabilidade |
|---|---|
| `simulator/src/mic1.c` e `.h` | Estado, ULA, deslocador, memória e quatro subciclos |
| `simulator/src/microcode.c` | Memória de controle; campos e microinstruções comentados |
| `simulator/src/isa.c` | Codificação e validação das palavras MAC-1 |
| `simulator/src/assembler.c` | Montador em C |
| `simulator/src/protocol.c` | Validação e serialização completa do estado |
| `simulator/src/server.c` | Servidor HTTP local em C e arquivos estáticos |
| `frontend/src/` | Interface React/TypeScript; não executa as instruções |
| `web/` | Interface já compilada, pronta para abrir pelo servidor |
| `bin/` | Executáveis prontos de Windows e Linux x86-64 |

Para modificar **C**: GCC/MinGW-w64 no Windows, depois `compilar.bat`.
No Linux/macOS com compilador C, `sh compilar.sh` ou `make`.
Essa recompilação aproveita a interface pronta e não precisa de Node.js.

Para modificar **a interface**: Node.js 22.12+ e npm, depois:

```sh
cd frontend
npm ci
npm run build
```

A compilação atualiza `web/`. Para desenvolvimento com recarga automática, inicie
o simulador na porta 3001 e rode `npm run dev` em `frontend/`.
O proxy envia as chamadas ao mesmo motor C.

## Limites da verificação

A validação foi realizada em Linux, com testes nativos, AddressSanitizer
(e sem o verificador de vazamentos) e UndefinedBehaviorSanitizer. O executável
Windows x86-64 foi compilado com MinGW-w64 e não depende de DLLs de desenvolvimento.
Não houve execução em Windows nativo nesta sessão; portanto não se promete ausência
absoluta de falhas em qualquer computador.
