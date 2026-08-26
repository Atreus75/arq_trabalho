// Assembler
/* 
Assembly em texto ---> binário 01
*/
#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

memInstruction memInstructionArray[MEM_INSTRUCTIONS] = {
	{0b0000, "lodd"},
	{0b0001, "stod"},
	{0b0010, "addd"},
	{0b0100, "jpos"},
	{0b0101, "jzer"},
	{0b0110, "jump"},
	{0b0111, "loco"},
	{0b1000, "lodl"},
	{0b1001, "stol"},
	{0b1010, "addl"},
	{0b1011, "subl"},
	{0b1100, "jneg"},
	{0b1101, "jnze"},
	{0b1110, "call"}
};

extendedInstruction extendedInstructionArray[EXTENDED_INSTRUCTIONS] = {
	{0b1111, "push"},
	{0b1111001, "popi"},
	{0b111101, "push"},
	{0b1111011, "pop"},
	{0b11111, "retn"},
	{0b1111101, "swap"},
	{0b11111100, "insp"},
	{0b1111111, "desp"},
	{0b11111111, "halt"}
};

errorStruct errorCodes = {
	.ArgumentError = 1
};

void raiseError(int code, int msgCount, ...){
	va_list msgs;
	va_start(msgs, msgCount);
	for (int c = 0; c<msgCount; c++){
		fprintf(stderr, va_arg(msgs, char *));
	}
	va_end(msgs);
	exit(code);
}

void processArguments(int argc, char * argv[], int * optionStates, char * outputPath){
// Argument processing function.
	char * usageMessage = {
		"usage: ./assembler [options]\n"
		"\t-o: Optional file output path. Ex: ./assembler -o output.bin\n"
		"\t-i: Interface method. Generates the binary content and sends for the web interface back-end. Use only with the web server interface.\n"
	};

	// Argument counting
	if (argc < 2) raiseError(errorCodes.ArgumentError, 2, "[-] Argument error: no argument provided.\n\n", usageMessage);	
	else if (argc > 4) raiseError(errorCodes.ArgumentError, 2, "[-] Argument error: bad argument sintax.\n\n", usageMessage);

	// argument parsing
	for (int c = 1; c < argc; c++){
		if (strcmp(argv[c], "-o") == 0 && !optionStates[0]){
			optionStates[0] = 1;
			if (argc <= (c+1) || strcmp(argv[c+1], "-i") == 0){
				raiseError(errorCodes.ArgumentError, 2, "[-] Argument error: no path provided for output.\n\n", usageMessage);
			}
			strcpy(outputPath, argv[++c]);
		}else if (strcmp(argv[c], "-i") == 0 && !optionStates[1]){
			optionStates[1] = 1;
		}
	}
}

int main(int argc, char * argv[]){
	// -o = 0 | -i = 1
	int optionStates[2] = {0, 0};
	char outputPath[255] = {0};
	processArguments(argc, argv, optionStates, outputPath);
	return 0;
}
