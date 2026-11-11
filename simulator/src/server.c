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

static int send_all(Socket s,const char *data,size_t n) {
    while (n) {
        int chunk=(int)(n>65536?65536:n); int sent=(int)send(s,data,chunk,0);
        if (sent<=0) return 0;
        data+=sent; n-=(size_t)sent;
    }
    return 1;
}
static void response(Socket s,int status,const char *mime,const char *body,size_t length) {
    char h[512];
    int n=snprintf(h,sizeof(h),"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\n\r\n",
        status,status==200?"OK":"Error",mime,length);
    if (send_all(s,h,(size_t)n)) send_all(s,body,length);
}
static void error_response(Socket s,int status,const char *message) {
    char *out=protocol_error(message);
    if (out) { response(s,status,"application/json; charset=utf-8",out,strlen(out)); free(out); }
}
static char *file_read(const char *path,size_t *size) {
    FILE *f=fopen(path,"rb"); if (!f) return NULL;
    if (fseek(f,0,SEEK_END)) { fclose(f); return NULL; }
    long n=ftell(f); if (n<0 || n>8*1024*1024) { fclose(f); return NULL; }
    rewind(f); char *data=malloc((size_t)n+1);
    if (!data) { fclose(f); return NULL; }
    size_t got=fread(data,1,(size_t)n,f); int bad=ferror(f); fclose(f);
    if (bad || got!=(size_t)n) { free(data); return NULL; }
    data[n]=0; *size=(size_t)n; return data;
}
static int safe_path(const char *p) {
    if (strstr(p,"..")) return 0;
    for (;*p;p++) if (!(isalnum((unsigned char)*p)||strchr("/_-.",*p))) return 0;
    return 1;
}
static void get_response(Socket s,const char *route) {
    char *data=NULL; size_t length=0;
    if (!strcmp(route,"/api/health")) {
        const char *ok="{\"ok\":true,\"engine\":\"C MIC-1 microprogramada\",\"version\":\"2.0\"}";
        response(s,200,"application/json; charset=utf-8",ok,strlen(ok)); return;
    }
    if (!strcmp(route,"/api/microcode")) {
        data=protocol_microcode(); response(s,200,"application/json; charset=utf-8",data,strlen(data)); free(data); return;
    }
    if (!safe_path(route)) { error_response(s,400,"Caminho invalido."); return; }
    char path[600]; snprintf(path,sizeof(path),"web%s",!strcmp(route,"/")?"/index.html":route);
    const char *ext=strrchr(path,'.'),*mime=NULL;
    if (ext) {
        if (!strcmp(ext,".html")) mime="text/html; charset=utf-8";
        if (!strcmp(ext,".js")) mime="text/javascript; charset=utf-8";
        if (!strcmp(ext,".css")) mime="text/css; charset=utf-8";
        if (!strcmp(ext,".svg")) mime="image/svg+xml";
        if (!strcmp(ext,".png")) mime="image/png";
        if (!strcmp(ext,".ico")) mime="image/x-icon";
    }
    if (!mime || !(data=file_read(path,&length))) { error_response(s,404,"Arquivo nao encontrado. Extraia a pasta web junto do executavel."); return; }
    response(s,200,mime,data,length); free(data);
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
    size_t used=0,headerSize=0,bodySize=0; char *end=NULL;
    while (used<16384) {
        int n=(int)recv(s,buf+used,(int)(16384-used),0);
        if (n<=0) goto finish;
        used+=(size_t)n; buf[used]=0; end=strstr(buf,"\r\n\r\n");
        if (end) { headerSize=(size_t)(end-buf)+4; break; }
    }
    if (!end) { error_response(s,400,"Cabecalho HTTP invalido."); goto finish; }
    char method[12],route[512],version[16];
    if (sscanf(buf,"%11s %511s %15s",method,route,version)!=3 || strncmp(version,"HTTP/",5)) { error_response(s,400,"HTTP invalido."); goto finish; }
    char *query=strchr(route,'?'); if (query) *query=0;
    int hasLength=0; char origin[256]="",host[256]="";
    char *line=strstr(buf,"\r\n")+2;
    while (line<end) {
        char *next=strstr(line,"\r\n"); if (!next) break;
        char saved=*next; *next=0;
        if (!strncasecmp(line,"Content-Length:",15)) {
            char *p=line+15; while (*p==' ') p++;
            char *tail; unsigned long n=strtoul(p,&tail,10);
            if (hasLength || !*p || *tail || n>MAX_REQUEST-headerSize) { *next=saved; error_response(s,413,"Pedido muito grande ou tamanho invalido."); goto finish; }
            bodySize=(size_t)n; hasLength=1;
        }
        if (!strncasecmp(line,"Transfer-Encoding:",18)) { *next=saved; error_response(s,400,"Use Content-Length."); goto finish; }
        if (!strncasecmp(line,"Origin:",7)) { const char *v=line+7; while (*v==' ')v++; snprintf(origin,sizeof(origin),"%s",v); }
        if (!strncasecmp(line,"Host:",5)) { const char *v=line+5; while (*v==' ')v++; snprintf(host,sizeof(host),"%s",v); }
        *next=saved; line=next+2;
    }
    /* Recusa acesso por nomes externos e POST vindo de outro site. */
    int localHost=!strcmp(host,"127.0.0.1") || !strcmp(host,"localhost") || !strncmp(host,"127.0.0.1:",10) || !strncmp(host,"localhost:",10);
    if (!localHost) { error_response(s,403,"Use localhost ou 127.0.0.1."); goto finish; }
    if (*origin) {
        char expected[280]; snprintf(expected,sizeof(expected),"http://%s",host);
        if (strcmp(origin,expected)) { error_response(s,403,"Origem nao permitida."); goto finish; }
    }
    while (used<headerSize+bodySize) {
        int n=(int)recv(s,buf+used,(int)(headerSize+bodySize-used),0);
        if (n<=0) goto finish;
        used+=(size_t)n;
    }
    buf[headerSize+bodySize]=0;
    if (!strcmp(method,"GET")) get_response(s,route);
    else if (!strcmp(method,"POST") && hasLength && !strncmp(route,"/api/",5)) {
        int status; char *out=protocol_request(route,buf+headerSize,&status);
        if (out) { response(s,status,"application/json; charset=utf-8",out,strlen(out)); free(out); }
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
int serve_local(int port,int openBrowser) {
#ifdef _WIN32
    WSADATA wsa; if (WSAStartup(MAKEWORD(2,2),&wsa)) return 1;
#else
    signal(SIGPIPE,SIG_IGN);
#endif
    FILE *index=fopen("web/index.html","rb");
    if (!index) { fprintf(stderr,"Pasta web ausente. Extraia o ZIP inteiro e abra iniciar.bat/iniciar.sh.\n"); return 1; }
    fclose(index);
    Socket listener=socket(AF_INET,SOCK_STREAM,0);
    if (listener==INVALID_SOCK) return 1;
    int yes=1;
#ifdef _WIN32
    setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&yes,sizeof(yes));
#else
    setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
#endif
    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET; addr.sin_port=htons((unsigned short)port); addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if (bind(listener,(struct sockaddr*)&addr,sizeof(addr)) || listen(listener,32)) {
        fprintf(stderr,"Nao foi possivel abrir a porta %d. Feche outra instancia ou use --port 3002.\n",port); close_socket(listener); return 1;
    }
    printf("\nMIC-1.sim — motor e montador em C\nAbra http://127.0.0.1:%d\nDeixe esta janela aberta. Ctrl+C encerra.\n",port); fflush(stdout);
#ifdef _WIN32
    if (openBrowser) { wchar_t url[100]; swprintf(url,100,L"http://127.0.0.1:%d",port); ShellExecuteW(NULL,L"open",url,NULL,NULL,SW_SHOWNORMAL); }
#else
    (void)openBrowser;
#endif
    for (;;) {
        Socket s=accept(listener,NULL,NULL); if (s==INVALID_SOCK) break;
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
    close_socket(listener); return 0;
}
