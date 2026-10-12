# Huffman File Compressor (C)

A command-line project implementing **lossless Huffman encoding and decoding in C**, with bit-packed binary output. Developed as a data structures and systems programming exercise.

## Features

- Counts the frequency of every possible byte (0–255), so binary as well as text files are supported.
- Constructs a Huffman binary tree and assigns variable-length prefix codes.
- Packs encoded bits into bytes (rather than storing ASCII `0` and `1` characters).
- Reconstructs the original file byte-for-byte using the stored frequency table.
- Handles empty files and single-symbol files.
- Rejects malformed headers and truncated streams in tested cases.

## Requirements

- A C11 compiler (Clang on macOS, or GCC)
- `make`
- `python3` for one binary test fixture (other tests use standard command-line tools)

## Build and run (macOS)

```bash
make
./huffman compress input.txt output.huff
./huffman decompress output.huff restored.txt
cmp input.txt restored.txt && echo "Round-trip passed"
```

Run the automated test script:

```bash
bash test.sh
```

To remove compiled artifacts:

```bash
make clean
```

## How it works

1. **Frequency counting:** Read the original file and count each byte's frequency.
2. **Tree construction:** Repeatedly merge the two lowest-frequency nodes into a parent node.
3. **Code assignment:** Traverse left for `0`, right for `1`. A leaf's path gives its prefix-free code.
4. **Bit packing:** Append individual code bits to an 8-bit buffer, write full bytes, and zero-pad the final byte.
5. **Decompression:** Rebuild the same tree from the stored frequencies, read bits from most-significant to least-significant, and stop when the original byte count has been reproduced.

## File format (HUF2)

This project's custom binary format is:

```text
4 bytes       ASCII magic: HUF2
2048 bytes    256 unsigned 64-bit frequency values, little-endian
Remaining     Huffman-coded payload, packed MSB-first, with zero padding
```

This version is **not compatible** with `.huff` files produced by the previous ASCII-bit implementation. Recompress files from their original source if needed.

The header has a fixed 2,052-byte overhead. This format favors simplicity and predictable decoding over optimal compression ratios; very small or high-entropy inputs can grow in size.

## Validation

`test.sh` runs round-trip file comparisons for sample text, empty input, one repeated byte, two unique symbols, all 256 possible byte values, and repeated text. It also checks that an invalid header and a truncated payload fail.

## Design notes and limitations

- Intended as an educational, single-threaded implementation, not a production archival format.
- The frequency table is fixed-size; future improvements could use a sparse or canonical-Huffman header to reduce overhead.
- A malformed payload that still decodes to the declared output length is not necessarily detected; adding a checksum would improve integrity checking.
- This version uses a simple linear-search tree builder instead of a priority queue; a min-heap is a natural future optimization.
