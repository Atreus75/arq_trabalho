#include <stdio.h>
#include <stdint.h>

int main(){
	FILE * fp = fopen("output.bin", "r");
	uint16_t instruction = 0;
	fread(&instruction, sizeof(uint16_t), 1, fp);
	printf("Instruction: %016b\n", instruction);
	fclose(fp);
	return 0;
}
