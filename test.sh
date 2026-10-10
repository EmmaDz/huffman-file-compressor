#!/bin/sh
set -eu
cd "$(dirname "$0")"
make
./huffman compress input.txt out.huff
./huffman decompress out.huff restored.txt
cmp input.txt restored.txt
printf '%s\n' 'Round-trip test passed: decoded output matches input.'
