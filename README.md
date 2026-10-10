# Huffman File Compressor (C)

A command-line project written in C that demonstrates Huffman coding for encoding and decoding file contents. It builds a frequency-based binary tree, generates prefix-free codes, writes the encoded content to a custom `.huff` file, and reconstructs the original data during decoding.

> **Implementation note:** This educational implementation stores encoded bits as ASCII `0` and `1` characters, preceded by a text-based frequency table. It demonstrates lossless coding but does **not** perform bit-packed compression and may produce files larger than the original. Empty input and single-symbol inputs are not currently supported reliably.

## Skills demonstrated

- Huffman coding and binary trees
- Frequency counting and depth-first traversal
- Pointers, dynamic memory allocation, and resource cleanup in C
- Binary-mode file I/O and command-line argument parsing
- Building with `make` and validating round-trip output

## Requirements

- macOS with Xcode Command Line Tools (`clang` and `make`)
- Alternatively, a compatible C11 compiler and `make`

On macOS, install the command-line tools if needed:

```bash
xcode-select --install
```

## Build

From the project folder:

```bash
make
```

This compiles `main.c` and `huffman.c` and produces the `huffman` executable.

## Usage

Encode a file:

```bash
./huffman compress input.txt out.huff
```

Decode it:

```bash
./huffman decompress out.huff restored.txt
```

Check that the decoded output matches the original:

```bash
cmp input.txt restored.txt && echo "Round-trip test passed"
```

Run the bundled smoke test:

```bash
bash test.sh
```

Remove build files:

```bash
make clean
```

## How it works

1. **Count frequencies:** Read the input bytes and count occurrences of each possible byte value.
2. **Build Huffman tree:** Repeatedly combine the two least frequent nodes into a parent node.
3. **Generate codes:** Walk the tree, assigning `0` for a left branch and `1` for a right branch.
4. **Encode:** Store the 256 frequency counts and a separator in the output header, followed by the encoded `0`/`1` text sequence.
5. **Decode:** Rebuild the tree from the stored frequencies and traverse it using the encoded sequence to recover the original bytes.

## Project layout

```text
.
├── main.c         # CLI entry point and argument parsing
├── huffman.c      # Tree construction, encoding, and decoding
├── huffman.h      # Data structures and function declarations
├── Makefile       # Build and clean targets
├── input.txt      # Sample input file
├── test.sh        # Basic round-trip smoke test
├── .gitignore     # Excludes build products and generated outputs
└── README.md
```

## Limitations and potential improvements

- Store encoded bits packed into bytes to provide actual space-saving compression.
- Handle empty files and single-distinct-byte files correctly.
- Validate malformed headers and truncated bitstreams more strictly.
- Add automated tests for larger, binary, and edge-case inputs.

## Author

Zida Chen
