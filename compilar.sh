#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
mkdir -p bin
cc -std=c11 -Wall -Wextra -Wpedantic -O2 -I simulator/src simulator/src/*.c -pthread -lm -o bin/mic1-local
cp -f bin/mic1-local bin/mic1-linux-x64
echo "Compilado: execute ./bin/mic1-local a partir desta pasta."
