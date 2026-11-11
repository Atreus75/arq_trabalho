; Passa 6 pela pilha e chama a função DOBRO.
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
