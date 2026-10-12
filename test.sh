#!/bin/sh
set -eu
cd "$(dirname "$0")"
make
TEMP_DIR=$(mktemp -d)
trap 'rm -rf "$TEMP_DIR"' EXIT HUP INT TERM
check_round_trip() {
  name=$1
  ./huffman compress "$TEMP_DIR/$name" "$TEMP_DIR/$name.huff"
  ./huffman decompress "$TEMP_DIR/$name.huff" "$TEMP_DIR/$name.restored"
  cmp "$TEMP_DIR/$name" "$TEMP_DIR/$name.restored"
  printf 'PASS: %s\n' "$name"
}
cp input.txt "$TEMP_DIR/sample"
check_round_trip sample
: > "$TEMP_DIR/empty"
check_round_trip empty
printf 'aaaaaaaaaaaaaaaaaaaa' > "$TEMP_DIR/single"
check_round_trip single
printf 'AB' > "$TEMP_DIR/two_symbols"
check_round_trip two_symbols
# A binary file containing every possible byte, including zero.
python3 -c 'import sys; sys.stdout.buffer.write(bytes(range(256)))' > "$TEMP_DIR/binary"
check_round_trip binary
# Repeated text demonstrates bit packing on a larger input.
awk 'BEGIN {for (i=0; i<2000; i++) printf "banana banana banana\n"}' > "$TEMP_DIR/repeated"
check_round_trip repeated
# A malformed header must fail, rather than producing success.
printf 'bad' > "$TEMP_DIR/invalid.huff"
if ./huffman decompress "$TEMP_DIR/invalid.huff" "$TEMP_DIR/bad_out"; then
    printf 'FAIL: malformed header accepted\n' >&2; exit 1
fi
printf 'PASS: invalid header rejected\n'
# A truncated stream should also be rejected.
printf 'abcabcabc' > "$TEMP_DIR/truncated"
./huffman compress "$TEMP_DIR/truncated" "$TEMP_DIR/full.huff"
head -c 2052 "$TEMP_DIR/full.huff" > "$TEMP_DIR/truncated.huff"
if ./huffman decompress "$TEMP_DIR/truncated.huff" "$TEMP_DIR/bad_out"; then
    printf 'FAIL: truncated stream accepted\n' >&2; exit 1
fi
printf 'PASS: truncated payload rejected\n'
printf 'All tests passed.\n'
