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
        char *input=malloc(1024*1024+1); if (!input) return 1;
        size_t n=fread(input,1,1024*1024,stdin); input[n]=0;
        if (!feof(stdin)) { free(input); return 1; }
        int status; char *out=protocol_request(argv[2],input,&status); free(input);
        if (!out) return 1;
        puts(out); free(out); return status==200?0:1;
    }
    int port=3001,openBrowser=1;
    for (int i=1;i<argc;i++) {
        if (!strcmp(argv[i],"--no-browser")) openBrowser=0;
        else if (!strcmp(argv[i],"--port") && i+1<argc) {
            char *end; long n=strtol(argv[++i],&end,10);
            if (*end || n<1024 || n>65535) { fprintf(stderr,"Porta invalida (1024..65535).\n"); return 1; }
            port=(int)n;
        } else { fprintf(stderr,"Use --port NUMERO, --no-browser ou --json ROTA.\n"); return 1; }
    }
#ifdef _WIN32
    /* Funciona mesmo ao abrir o executavel em bin/ com duplo clique. */
    wchar_t path[32768]; DWORD n=GetModuleFileNameW(NULL,path,32768);
    if (n && n<32768) {
        wchar_t *slash=wcsrchr(path,L'\\');
        if (slash) { *slash=0; slash=wcsrchr(path,L'\\'); if (slash && !_wcsicmp(slash+1,L"bin")) *slash=0; SetCurrentDirectoryW(path); }
    }
#endif
    return serve_local(port,openBrowser);
}
