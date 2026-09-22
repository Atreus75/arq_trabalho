# Conjunto de Instruções MAC-1 (ISA)

A arquitetura MAC-1 opera com palavras de 16 bits. Cada instrução convencional é composta por um opcode de 4 bits (bits 15 a 12) e um campo de operando de 12 bits (bits 11 a 0). O espaço de endereçamento direto compreende $2^{12} = 4096$ palavras.

## 1. Formato da Palavra de Instrução

### 1.1 Instruções de Acesso à Memória (Convencionais)
```
 15        12 11                               0
+------------+----------------------------------+
|   Opcode   |        Operando (Endereço)       |
|   (4 bits) |             (12 bits)            |
+------------+----------------------------------+
```

### 1.2 Instruções Estendidas
Instruções estendidas utilizam o prefixo `1111` (0xF) no opcode principal. O tipo de operação é determinado pelos bits subsequentes:
```
 15        12 11   8 7                         0
+------------+------+---------------------------+
|    1111    | Sub- |         Não usado         |
|   (4 bits) | code |                           |
+------------+------+---------------------------+
```

---

## 2. Tabela de Instruções Convencionais (Acesso à Memória)

| Mnemônico | Opcode (Binário) | Opcode (Hex) | Operação Algébrica / Efeito no Estado |
| :--- | :--- | :--- | :--- |
| `lodd` | `0000` | `0x0` | $\text{AC} \leftarrow \text{mem}[\text{operando}]$ |
| `stod` | `0001` | `0x1` | $\text{mem}[\text{operando}] \leftarrow \text{AC}$ |
| `addd` | `0010` | `0x2` | $\text{AC} \leftarrow \text{AC} + \text{mem}[\text{operando}]$ |
| `subd` | `0011` | `0x3` | $\text{AC} \leftarrow \text{AC} - \text{mem}[\text{operando}]$ |
| `jpos` | `0100` | `0x4` | $\text{se } \text{AC} \ge 0 \implies \text{PC} \leftarrow \text{operando}$ |
| `jzer` | `0101` | `0x5` | $\text{se } \text{AC} = 0 \implies \text{PC} \leftarrow \text{operando}$ |
| `jump` | `0110` | `0x6` | $\text{PC} \leftarrow \text{operando}$ |
| `loco` | `0111` | `0x7` | $\text{AC} \leftarrow \text{operando}$ |
| `lodl` | `1000` | `0x8` | $\text{AC} \leftarrow \text{mem}[\text{SP} + \text{operando}]$ |
| `stol` | `1001` | `0x9` | $\text{mem}[\text{SP} + \text{operando}] \leftarrow \text{AC}$ |
| `addl` | `1010` | `0xA` | $\text{AC} \leftarrow \text{AC} + \text{mem}[\text{SP} + \text{operando}]$ |
| `subl` | `1011` | `0xB` | $\text{AC} \leftarrow \text{AC} - \text{mem}[\text{SP} + \text{operando}]$ |
| `jneg` | `1100` | `0xC` | $\text{se } \text{AC} < 0 \implies \text{PC} \leftarrow \text{operando}$ |
| `jnze` | `1101` | `0xD` | $\text{se } \text{AC} \ne 0 \implies \text{PC} \leftarrow \text{operando}$ |
| `call` | `1110` | `0xE` | $\text{SP} \leftarrow \text{SP} - 1;\; \text{mem}[\text{SP}] \leftarrow \text{PC};\; \text{PC} \leftarrow \text{operando}$ |

---

## 3. Tabela de Instruções Estendidas

| Mnemônico | Código Estendido (Binário) | Operação Algébrica / Efeito no Estado |
| :--- | :--- | :--- |
| `push` | `1111 0100 0000 0000` | $\text{SP} \leftarrow \text{SP} - 1;\; \text{mem}[\text{SP}] \leftarrow \text{AC}$ |
| `pop`  | `1111 0110 0000 0000` | $\text{AC} \leftarrow \text{mem}[\text{SP}];\; \text{SP} \leftarrow \text{SP} + 1$ |
| `popi` | `1111 0010 0000 0000` | $\text{mem}[\text{AC}] \leftarrow \text{mem}[\text{SP}];\; \text{SP} \leftarrow \text{SP} + 1$ |
| `retn` | `1111 1000 0000 0000` | $\text{PC} \leftarrow \text{mem}[\text{SP}];\; \text{SP} \leftarrow \text{SP} + 1$ |
| `swap` | `1111 1010 0000 0000` | Troca $\text{AC} \leftrightarrow \text{SP}$ |
| `insp` | `1111 1100 <operando>`| $\text{SP} \leftarrow \text{SP} + \text{offset}$ |
| `desp` | `1111 1110 <operando>`| $\text{SP} \leftarrow \text{SP} - \text{offset}$ |
| `halt` | `1111 1111 1111 1111` | Interrompe o ciclo de busca (`running = 0`) |

