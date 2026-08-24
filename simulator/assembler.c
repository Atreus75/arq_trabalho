// Assembler
/* 
Assembly em texto ---> binário 01
*/
#include "assembler.h"


mem_instruction MEM_INSTRUCTION_ARRAY[MEM_INSTRUCTIONS] = {
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

extended_instruction EXTENDED_INSTRUCTION_ARRAY[EXTENDED_INSTRUCTIONS] = {
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

int main(){
	return 0; //coloquei isso aqui pro GCC não reclamar
}
