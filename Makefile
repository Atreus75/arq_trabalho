CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
BIN_DIR = bin
SIM_DIR = simulator

all: directories $(BIN_DIR)/assembler $(BIN_DIR)/simulator

directories:
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/assembler: $(SIM_DIR)/assembler.c $(SIM_DIR)/assembler.h
	$(CC) $(CFLAGS) $(SIM_DIR)/assembler.c -o $(BIN_DIR)/assembler

$(BIN_DIR)/simulator: $(SIM_DIR)/simulator.c $(SIM_DIR)/simulator.h
	$(CC) $(CFLAGS) -c $(SIM_DIR)/simulator.c -o $(BIN_DIR)/simulator.o

clean:
	rm -rf $(BIN_DIR)

.PHONY: all directories clean

