# Especificação da API e Protocolo IPC

Este documento define os contratos de comunicação entre a interface web (Frontend), o servidor intermediário (Node.js) e os executáveis em C (`assembler` e `simulator`).

---

## 1. Endpoints HTTP (REST)

### 1.1 `POST /api/assemble`
Recebe código fonte Assembly MAC-1 em texto e executa o montador.

#### Requisição
```json
{
  "source": "lodd 10\naddd 11\nstod 12\nhalt"
}
```

#### Resposta de Sucesso (`200 OK`)
```json
{
  "status": "success",
  "binary_hex": ["000A", "200B", "100C", "FFFF"],
  "word_count": 4,
  "errors": []
}
```

#### Resposta de Falha de Compilação (`400 Bad Request`)
```json
{
  "status": "error",
  "binary_hex": [],
  "word_count": 0,
  "errors": [
    {
      "line": 1,
      "code": 3,
      "message": "Invalid Instruction: instruction not found"
    }
  ]
}
```

---

### 1.2 `POST /api/simulate/step`
Avança exatamente um ciclo de instrução no simulador.

#### Requisição
```json
{
  "session_id": "uuid-v4",
  "action": "STEP"
}
```

#### Resposta (`200 OK`)
```json
{
  "pc": 1,
  "ac": 42,
  "sp": 4095,
  "ir": "0x000A",
  "zero_flag": false,
  "neg_flag": false,
  "running": true,
  "memory_deltas": [
    {
      "address": 10,
      "value": 42
    }
  ]
}
```

---

## 2. Protocolo WebSocket (`/ws/simulate`)

Canal bidirecional para depuração e execução contínua.

### 2.1 Mensagens do Cliente para o Servidor (Comandos)

| Comando | Payload | Descrição |
| :--- | :--- | :--- |
| `LOAD` | `{ "binary": ["000A", "200B"] }` | Carrega o vetor de palavras na memória a partir do endereço `0x000` e executa reset. |
| `STEP` | `{}` | Executa um único ciclo de instrução e retorna o estado. |
| `RUN` | `{ "clock_ms": 100 }` | Executa continuamente com intervalo definido até encontrar `halt`. |
| `PAUSE`| `{}` | Interrompe o laço de execução contínua. |
| `RESET`| `{}` | Zera registradores e recarrega o estado inicial da memória. |

### 2.2 Mensagens do Servidor para o Cliente (Eventos de Estado)

#### Formato do Frame
```json
{
  "event": "STATE_UPDATE",
  "data": {
    "pc": 2,
    "ac": 15,
    "sp": 4095,
    "running": 1,
    "memory_deltas": [
      { "address": 12, "value": 15 }
    ]
  }
}
```

#### Evento de Parada (`HALT`)
```json
{
  "event": "HALTED",
  "data": {
    "pc": 4,
    "ac": 15,
    "sp": 4095,
    "reason": "HALT_INSTRUCTION"
  }
}
```

