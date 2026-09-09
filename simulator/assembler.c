// Assembler
/* 
Assembly em texto ---> binário 01
*/
#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

memInstruction memInstructionArray[MEM_INSTRUCTIONS] = {
	{0b0000, "lodd"},
	{0b0001, "stod"},
	{0b0010, "addd"},
	{0b0011, "subd"},
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
	.ArgumentError = 1,
	.InvalidOperand = 2,
	.InvalidInstruction = 3
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

int isNumeric(char * str){
	int pos = 0;
	for (; (str[pos] != '\0') && (str[pos] >= '0' && str[pos] <= '9'); pos++);
	if (str[pos] == '\0') return 1;
	return 0;
}

int isInstruction(char * str){
	for (int c = 0; c < MEM_INSTRUCTIONS; c++){
		if (strcmp(str, memInstructionArray[c].name) == 0) return memInstructionArray[c].opcode;
	}
	return -1;
}

void processArguments(int argc, char * argv[], int * optionStates, char * outputPath){
// Argument processing function.
	char * usageMessage = {
		"usage: ./assembler [options]\n"
		"\t-o: Optional file output path. Ex: ./assembler -o output.bin\n"
		"\t-i: Interface mode. Generates the binary content and sends for the web interface back-end. Use only with the web server interface.\n"
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
			c++;
			strcpy(outputPath, argv[c]);
		}else if (strcmp(argv[c], "-i") == 0 && !optionStates[1]){
			optionStates[1] = 1;
		}
	}
}

uint16_t assembleLine(char * line){
	char currentToken[5] = {0};
	int linePos = 0, instructionFound = 0, opcode = 0, operand = 0, tokenPos = 0;
	uint16_t instruction = 0;
	while (1){
		char c = line[linePos];
		if (c == ' ' || c == '\n' || c == '\0'){
			if (!instructionFound){
				tokenPos = 0;
				opcode = isInstruction(currentToken);
				if (opcode < 0){
					raiseError(errorCodes.InvalidOperand, 3, "[-] Invalid Instruction: instruction not found at line \"", line, "\"\n\n");
				}
				instructionFound = 1;
				memset(currentToken, '\0', 5);
				continue;
			}else{
				if ( !(isNumeric(currentToken)) ){
					raiseError(errorCodes.InvalidOperand, 3, "[-] Operand Error: non numeric operand at line \"", line, "\"\n\n");
				}
				operand = atoi(currentToken);
				if (operand > 4095 || operand < 0){
					raiseError(errorCodes.InvalidOperand, 3, "[-] Operand Error: out of range operand at line \"", line, "\"\n\n");
				}
			}
			if (c == '\0') break;
		}else{
			currentToken[tokenPos] = c;
			tokenPos++;
		}
		linePos++;
	}
	instruction = (opcode << 12) | (operand & 0xFFF);
	printf("Instruction: %b\nOperand: %b\n", opcode, operand);
	return instruction;	
}

int main(int argc, char * argv[]){
	// -o = 0 | -i = 1
	int optionStates[2] = {0, 0};
	char outputPath[255] = {0};
	processArguments(argc, argv, optionStates, outputPath);
	uint16_t instruction = assembleLine("addd 38");
	FILE * fp = fopen(outputPath, "wb");
	fwrite(&instruction, sizeof(uint16_t), 1, fp);
	fclose(fp);
	return 0;
}

