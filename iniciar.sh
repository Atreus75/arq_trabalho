#!/bin/sh
set -eu
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) chmod +x bin/mic1-linux-x64; exec ./bin/mic1-linux-x64 "$@" ;;
  *) sh compilar.sh; exec ./bin/mic1-local "$@" ;;
esac
