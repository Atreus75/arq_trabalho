CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
SRC = $(wildcard simulator/src/*.c)
HEADERS = $(wildcard simulator/src/*.h)

.PHONY: all clean frontend
all: bin/mic1-local
bin/mic1-local: $(SRC) $(HEADERS)
	mkdir -p bin
	$(CC) $(CFLAGS) -I simulator/src $(SRC) -pthread -lm -o $@
	cp -f bin/mic1-local bin/mic1-linux-x64
clean:
	rm -f bin/mic1-local bin/mic1-linux-x64
frontend:
	cd frontend && npm ci && npm run build
