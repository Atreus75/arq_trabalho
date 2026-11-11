#ifndef MIC1_PROTOCOL_H
#define MIC1_PROTOCOL_H
char *protocol_request(const char *route,const char *body,int *status);
char *protocol_error(const char *message);
char *protocol_microcode(void);
#endif
