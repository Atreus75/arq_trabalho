export const ARRAY_SOURCE = `; Exemplo: "se (i <= n) entao m := a[i]"
; Mesmo padrao de acesso a array indexado usado em aula.
; a[] comeca no endereco 200; i e n sao variaveis em memoria.

        LOCO 5
        STOD 100      ; n := 5
        LOCO 3
        STOD 101      ; i := 3
        LOCO 77
        STOD 203      ; a[3] := 77  (so pra ter algo pra ler)

        LODD 100      ; ac := n
        SUBD 101      ; ac := n - i
        JNEG FIM      ; se (n - i) < 0, ou seja i > n, pula

        LOCO 200      ; ac := endereco base de a
        ADDD 101      ; ac := endereco_base(a) + i  = endereco de a[i]
        PSHI          ; empilha m[ac], ou seja, o valor a[i]
        POP           ; ac := a[i]
        STOD 102      ; m := a[i]

FIM:    JUMP FIM
`;

export const EXAMPLES = [
  { name: "Soma simples", source: `; 7 + 5 = 12. O resultado fica em m[100].
        LOCO 7
        STOD 100
        LOCO 5
        ADDD 100
        STOD 100
END:    JUMP END
` },
  { name: "Função e pilha", source: `; Passa 6 pela pilha e chama a função DOBRO.
; Ao terminar: AC = 12, m[100] = 12, SP = 4096.
        LOCO 6
        PUSH
        CALL DOBRO
        INSP 1       ; remove o argumento da pilha
        STOD 100
END:    JUMP END

DOBRO:  LODL 1       ; argumento, acima do retorno
        ADDL 1
        RETN
` },
  { name: "Array da aula", source: ARRAY_SOURCE },
  { name: "Fibonacci da referência", source: `; Cálculo da referência com variáveis implícitas e término em END: JUMP END.
; Os termos ficam na pilha. Pausar no endereço 16 permite ver o final.
START:LOCO 16
STOD var1
LOCO 1
STOD var2
LOCO 0
PUSH
LOCO 1
PUSH
LOOP:LODL 1
ADDL 0
PUSH
LODD var1
SUBD var2
STOD var1
JZER END
JUMP LOOP
END:JUMP END
` },
];
