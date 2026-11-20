#include "protocol.h"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
int main(int argc,char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc>2 && !strcmp(argv[1],"--json")) {
        char *entrada=malloc(1024*1024+1); if (!entrada) return 1;
        size_t n=fread(entrada,1,1024*1024,stdin); entrada[n]=0;
        if (!feof(stdin)) { free(entrada); return 1; }
        int status; char *saida=protocol_request(argv[2],entrada,&status); free(entrada);
        if (!saida) return 1;
        puts(saida); free(saida); return status==200?0:1;
    }
    int porta=3001,abrirNavegador=1;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--no-browser")) abrirNavegador=0;
        else if (!strcmp(argv[i],"--port") && i+1<argc) {
            char *fim; long n=strtol(argv[++i],&fim,10);
            if (*fim || n<1024 || n>65535) { fprintf(stderr,"Porta invalida (1024..65535).\n"); return 1; }
            porta=(int)n;
        } else { fprintf(stderr,"Use --port NUMERO, --no-browser ou --json ROTA.\n"); return 1; }
    }
#ifdef _WIN32
    /* Funciona mesmo ao abrir o executavel em bin/ com duplo clique. */
    wchar_t caminho[32768]; DWORD n=GetModuleFileNameW(NULL,caminho,32768);
    if (n && n<32768) {
        wchar_t *barra=wcsrchr(caminho,L'\\');
        if (barra) { *barra=0; barra=wcsrchr(caminho,L'\\'); if (barra && !_wcsicmp(barra+1,L"bin")) *barra=0; SetCurrentDirectoryW(caminho); }
    }
#endif
    return serve_local(porta,abrirNavegador);
}
