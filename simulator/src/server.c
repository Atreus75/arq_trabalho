/* Servidor local em C. Serve a interface pronta e chama diretamente o motor.
 * Apenas loopback, sem dependencias externas, sem executar comandos do usuario.
 */
#define _POSIX_C_SOURCE 200809L
#include "server.h"
#include "protocol.h"
#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <shellapi.h>
typedef SOCKET Socket;
#define INVALID_SOCK INVALID_SOCKET
#define close_socket closesocket
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <strings.h>
typedef int Socket;
#define INVALID_SOCK (-1)
#define close_socket close
#endif
#define MAX_REQUEST (1024*1024)

static int send_all(Socket s,const char *dados,size_t n) {
    while (n) {
        int bloco=(int)(n>65536?65536:n); int enviados=(int)send(s,dados,bloco,0);
        if (enviados<=0) return 0;
        dados+=enviados; n-=(size_t)enviados;
    }
    return 1;
}
static void response(Socket s,int status,const char *mime,const char *corpo,size_t length) {
    char h[512];
    int n=snprintf(h,sizeof(h),"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n\r\n",
        status,status==200?"OK":"Error",mime,length);
    if (send_all(s,h,(size_t)n)) send_all(s,corpo,length);
}
static void error_response(Socket s,int status,const char *msg) {
    char *saida=protocol_error(msg);
    if (saida) { response(s,status,"application/json; charset=utf-8",saida,strlen(saida)); free(saida); }
}
static char *file_read(const char *caminho,size_t *tam) {
    FILE *f=fopen(caminho,"rb"); if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long n=ftell(f); if (n<0 || n>8*1024*1024) { fclose(f); return NULL; }
    rewind(f); char *dados=malloc((size_t)n+1);
    if (!dados) { fclose(f); return NULL; }
    size_t lidos=fread(dados,1,(size_t)n,f); int erroLeitura=ferror(f); fclose(f);
    if (erroLeitura || lidos!=(size_t)n) { free(dados); return NULL; }
    dados[n]=0; *tam=(size_t)n; return dados;
}
static int safe_path(const char *p) {
    if (strstr(p,"..")) return 0;
    for (;*p;p++) if (!(isalnum((unsigned char)*p)||strchr("/_-.",*p))) return 0;
    return 1;
}
static void get_response(Socket s,const char *rota) {
    char *dados=NULL; size_t length=0;
    if (!strcmp(rota,"/api/health")) {
        const char *ok="{\"ok\":true,\"engine\":\"C MIC-1 microprogramada\",\"version\":\"2.0\"}";
        response(s,200,"application/json; charset=utf-8",ok,strlen(ok)); return;
    }
    if (!strcmp(rota,"/api/microcode")) {
        dados=protocol_microcode(); response(s,200,"application/json; charset=utf-8",dados,strlen(dados)); free(dados); return;
    }
    if (!safe_path(rota)) { error_response(s,400,"Caminho invalido."); return; }
    char caminho[600]; snprintf(caminho,sizeof(caminho),"web%s",!strcmp(rota,"/")?"/index.html":rota);
    const char *ext=strrchr(caminho,'.'),*mime=NULL;
    if (ext) {
        if (!strcmp(ext,".html")) mime="text/html; charset=utf-8";
        if (!strcmp(ext,".js")) mime="text/javascript; charset=utf-8";
        if (!strcmp(ext,".css")) mime="text/css; charset=utf-8";
        if (!strcmp(ext,".svg")) mime="image/svg+xml";
        if (!strcmp(ext,".png")) mime="image/png";
        if (!strcmp(ext,".ico")) mime="image/x-icon";
    }
    if (!mime || !(dados=file_read(caminho,&length))) { error_response(s,404,"Arquivo nao encontrado. Extraia a pasta web junto do executavel."); return; }
    response(s,200,mime,dados,length); free(dados);
}
static void handle(Socket s) {
#ifdef _WIN32
    DWORD timeout=5000; setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
    setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
#else
    struct timeval timeout={5,0}; setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
#endif
    char *buf=malloc(MAX_REQUEST+1); if (!buf) return;
    size_t usado=0,tamCabecalho=0,tamCorpo=0; char *fim=NULL;
    while (usado<16384) {
        int n=(int)recv(s,buf+usado,(int)(16384-usado),0);
        if (n<=0) goto finish;
        usado+=(size_t)n; buf[usado]=0; fim=strstr(buf,"\r\n\r\n");
        if (fim) { tamCabecalho=(size_t)(fim-buf)+4; break; }
    }
    if (!fim) { error_response(s,400,"Cabecalho HTTP invalido."); goto finish; }
    char metodo[12],rota[512],versao[16];
    if (sscanf(buf,"%11s %511s %15s",metodo,rota,versao)!=3 || strncmp(versao,"HTTP/",5)) { error_response(s,400,"HTTP invalido."); goto finish; }
    char *query=strchr(rota,'?'); if (query) *query=0;
    int hasLength=0; char origem[256]="",host[256]="";
    char *linha=strstr(buf,"\r\n")+2;
    while (linha<fim) {
        char *prox=strstr(linha,"\r\n"); if (!prox) break;
        char salvo=*prox; *prox=0;
        if (!strncasecmp(linha,"Content-Length:",15)) {
            char *p=linha+15; while (*p==' ') p++;
            char *resto; unsigned long n=strtoul(p,&resto,10);
            if (hasLength || !*p || *resto || n>MAX_REQUEST-tamCabecalho) { *prox=salvo; error_response(s,413,"Pedido muito grande ou tamanho invalido."); goto finish; }
            tamCorpo=(size_t)n; hasLength=1;
        }
        if (!strncasecmp(linha,"Transfer-Encoding:",18)) { *prox=salvo; error_response(s,400,"Use Content-Length."); goto finish; }
        if (!strncasecmp(linha,"Origin:",7)) { const char *v=linha+7; while (*v==' ')v++; snprintf(origem,sizeof(origem),"%s",v); }
        if (!strncasecmp(linha,"Host:",5)) { const char *v=linha+5; while (*v==' ')v++; snprintf(host,sizeof(host),"%s",v); }
        *prox=salvo; linha=prox+2;
    }
    /* Recusa acesso por nomes externos e POST vindo de outro site. */
    int ehLocal=!strcmp(host,"127.0.0.1") || !strcmp(host,"localhost") || !strncmp(host,"127.0.0.1:",10) || !strncmp(host,"localhost:",10);
    if (!ehLocal) { error_response(s,403,"Use localhost ou 127.0.0.1."); goto finish; }
    if (*origem) {
        char esperado[280]; snprintf(esperado,sizeof(esperado),"http://%s",host);
        if (strcmp(origem,esperado)) { error_response(s,403,"Origem nao permitida."); goto finish; }
    }
    while (usado<tamCabecalho+tamCorpo) {
        int n=(int)recv(s,buf+usado,(int)(tamCabecalho+tamCorpo-usado),0);
        if (n<=0) goto finish;
        usado+=(size_t)n;
    }
    buf[tamCabecalho+tamCorpo]=0;
    if (!strcmp(metodo,"GET")) get_response(s,rota);
    else if (!strcmp(metodo,"POST") && hasLength && !strncmp(rota,"/api/",5)) {
        int status; char *saida=protocol_request(rota,buf+tamCabecalho,&status);
        if (saida) { response(s,status,"application/json; charset=utf-8",saida,strlen(saida)); free(saida); }
    } else error_response(s,405,"Metodo nao suportado.");
finish:
    free(buf);
}
#ifdef _WIN32
static DWORD WINAPI connection(LPVOID arg) {
#else
static void *connection(void *arg) {
#endif
    Socket s=*(Socket*)arg; free(arg); handle(s); close_socket(s); return 0;
}
int serve_local(int porta,int abrirNavegador) {
#ifdef _WIN32
    WSADATA wsa; if (WSAStartup(MAKEWORD(2,2),&wsa)) return 1;
#else
    signal(SIGPIPE,SIG_IGN);
#endif
    FILE *index=fopen("web/index.html","rb");
    if (!index) { fprintf(stderr,"Pasta web ausente. Extraia o ZIP inteiro e abra iniciar.bat/iniciar.sh.\n"); return 1; }
    fclose(index);
    Socket escuta=socket(AF_INET,SOCK_STREAM,0);
    if (escuta==INVALID_SOCK) return 1;
    int sim=1;
#ifdef _WIN32
    setsockopt(escuta,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&sim,sizeof(sim));
#else
    setsockopt(escuta,SOL_SOCKET,SO_REUSEADDR,&sim,sizeof(sim));
#endif
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET; addr.sin_port=htons((unsigned short)porta); addr.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(escuta,(struct sockaddr*)&addr,sizeof(addr)) || listen(escuta,32)) {
        fprintf(stderr,"Nao foi possivel abrir a porta %d. Feche outra instancia ou use --port 3002.\n",porta); close_socket(escuta); return 1;
    }
    printf("\nMIC-1.sim — motor e montador em C\nAbra http://127.0.0.1:%d\nDeixe esta janela aberta. Ctrl+C encerra.\n",porta); fflush(stdout);
#ifdef _WIN32
    if (abrirNavegador) { wchar_t url[100]; swprintf(url,100,L"http://127.0.0.1:%d",porta); ShellExecuteW(NULL,L"open",url,NULL,NULL,SW_SHOWNORMAL); }
#else
    (void)abrirNavegador;
#endif
    for (;;) {
        Socket s=accept(escuta,NULL,NULL); if (s==INVALID_SOCK) break;
        Socket *arg=malloc(sizeof(*arg)); if (!arg) { close_socket(s); continue; } *arg=s;
#ifdef _WIN32
        HANDLE thread=CreateThread(NULL,0,connection,arg,0,NULL);
        if (thread) CloseHandle(thread); else { free(arg); close_socket(s); }
#else
        pthread_t thread;
        if (!pthread_create(&thread,NULL,connection,arg)) pthread_detach(thread);
        else { free(arg); close_socket(s); }
#endif
    }
    close_socket(escuta); return 0;
}
