// MACROS
#define EXTENDED_INSTRUCTIONS 9
#define MEM_INSTRUCTIONS 14

typedef struct {
	int opcode;
	char name[5];
} mem_instruction;

typedef mem_instruction extended_instruction;

