// MACROS
#define EXTENDED_INSTRUCTIONS 9
#define MEM_INSTRUCTIONS 15

// TIPOS

typedef struct {
	int opcode;
	char name[5];
} memInstruction;

typedef memInstruction extendedInstruction;

typedef struct {
	int ArgumentError; 
	int InvalidOperand;
	int InvalidInstruction;
} errorStruct;

// FUNÇÕES 
void raiseError(int code, int msgCount, ...);
void processArguments(int argc, char * argv[], int * optionStates, char * outputPath);
int isNumeric(char * str); // Returns 1 if the given string is numeric, and 0 if not
int isInstruction(char * str);

