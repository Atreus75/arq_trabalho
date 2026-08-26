// MACROS
#define EXTENDED_INSTRUCTIONS 9
#define MEM_INSTRUCTIONS 14

// TIPOS

typedef struct {
	int opcode;
	char name[5];
} memInstruction;

typedef memInstruction extendedInstruction;

typedef struct {
	int ArgumentError; 
} errorStruct;

// FUNÇÕES 
void raiseError(int code, int msgCount, ...);
void processArguments(int argc, char * argv[], int * optionStates, char * outputPath);

