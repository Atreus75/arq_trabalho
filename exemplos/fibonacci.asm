; Cálculo da referência, com variáveis implícitas e HALT no final.
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
