; Exemplo: "se (i <= n) entao m := a[i]"
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
