# Documentação do Assembler
## MACROS (defines)
### #define MEM_INSTRUCTIONS 14
Quantidade de instruções comuns (de acesso á memória) suportadas no momento.

### #define EXTENDED_INSTRUCTIONS 9
Quantidade de instruções estendidas suportadas no momento.

## Tipos
### memInstruction
Struct criado para auxiliar na referência que temos acerca das instruções que conhecemos, em tempo de execução. Cada struct deste tipo deve representar uma instrução.
```C
typedef struct {
	int opcode;
	char name[5];
} memInstruction;
```
### extendedInstruction
Exatamente igual ao anterior, com a diferença apenas do nome, para que possamos diferenciar instruções comuns das **instruções extendidas**, que possuem *opcodes* maiores.
```C
typedef memInstruction extendedInstruction;
```
### errorStruct
Struct criado para organizar os códigos de retorno utilizados em cada caso de erro. Retornar códigos de erro personalizados com `return [codigo]` nos ajuda a debugar melhor o programa, pois podemos saber exatamente que tipo de erro ocorreu.
```C
typedef struct {
	int ArgumentError; 
} errorStruct;
```

## Funções
### void raiseError(int code, int msgCount, ...)
Utilizada para printar mensagens de erro e retornar o código de erro adequado. Dá *exit(code)* ao final da execução.
#### Parâmetros
* **int code**: O código de erro a ser retornado.
* **int msgCount**: A quantidade de strings (*char **) de erro a serem imprimidas.
* **...**: Todas as mensagens de erro a serem impressas. A quantidade é definida apenas por msgCount.

### void processArguments(int argc, char * argv[], int * optionStates, char * outputPath)
Utilizada para processar os argumentos de linha de comando do programa.
#### Parâmetros
* **int argc**: Quantidade de argumentos a serem analisados.
* **char * argv[]**: Array de strings contendo cara parâmetro.
* **int * optionStates**: Array de inteiros onde cada posição representa o estado de um parâmetro. As posições são 0 para "-o" e 1 para "-i" no momento. O valor 0 em cada posição significa que o argumento não foi encontrado, enquanto o valor 1 simboliza que o argumento foi encontrado.
* **outputPath**: Ponteiro de *char* onde será salvo o caminho especificado após a opção "-o".
