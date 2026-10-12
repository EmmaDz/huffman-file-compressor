#include "huffman.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* HUF2 is a new format: NOT compatible with the previous ASCII-bit files.
   Header: 4-byte magic "HUF2", then 256 uint64 frequencies (little-endian).
   Body: packed Huffman bits, most significant bit first, padded with zeros. */
#define SYMBOLS 256
#define MAX_NODES (2 * SYMBOLS - 1)
static const unsigned char MAGIC[4] = {'H', 'U', 'F', '2'};

typedef struct Node {
    unsigned char symbol;
    uint64_t frequency;
    struct Node *left;
    struct Node *right;
} Node;

typedef struct {
    unsigned char bits[SYMBOLS];
    int length;
} Code;

static void free_tree(Node *node) {
    if (!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

static Node *new_node(unsigned char symbol, uint64_t frequency, Node *left, Node *right) {
    Node *node = malloc(sizeof(*node));
    if (!node) return NULL;
    node->symbol = symbol;
    node->frequency = frequency;
    node->left = left;
    node->right = right;
    return node;
}

static int is_leaf(const Node *node) { return node && !node->left && !node->right; }

static int find_min(Node *nodes[SYMBOLS], int count) {
    int best = -1;
    for (int i = 0; i < count; i++) {
        if (nodes[i] && (best < 0 || nodes[i]->frequency < nodes[best]->frequency))
            best = i;
    }
    return best;
}

/* Rebuild deterministically from the 256 stored frequencies. */
static Node *build_tree(const uint64_t frequency[SYMBOLS], int *ok) {
    Node *nodes[SYMBOLS] = {0};
    int count = 0;
    *ok = 0;
    for (int i = 0; i < SYMBOLS; i++) {
        if (!frequency[i]) continue;
        nodes[count] = new_node((unsigned char)i, frequency[i], NULL, NULL);
        if (!nodes[count]) goto error;
        count++;
    }
    if (!count) { *ok = 1; return NULL; } /* Valid empty file. */
    while (1) {
        int first = find_min(nodes, count);
        Node *a = nodes[first];
        nodes[first] = NULL;
        int second = find_min(nodes, count);
        if (second < 0) { *ok = 1; return a; }
        Node *b = nodes[second];
        nodes[second] = NULL;
        if (UINT64_MAX - a->frequency < b->frequency) {
            free_tree(a);
            free_tree(b);
            goto error;
        }
        Node *parent = new_node(0, a->frequency + b->frequency, a, b);
        if (!parent) {
            free_tree(a);
            free_tree(b);
            goto error;
        }
        nodes[first] = parent;
    }
error:
    for (int i = 0; i < count; i++) free_tree(nodes[i]);
    return NULL;
}

static void make_codes(const Node *node, Code codes[SYMBOLS],
                       unsigned char path[SYMBOLS], int depth) {
    if (!node) return;
    if (is_leaf(node)) {
        /* A single-symbol input needs no data bits: frequency tells us the length. */
        Code *code = &codes[node->symbol];
        code->length = depth;
        memcpy(code->bits, path, (size_t)depth);
        return;
    }
    path[depth] = 0;
    make_codes(node->left, codes, path, depth + 1);
    path[depth] = 1;
    make_codes(node->right, codes, path, depth + 1);
}

static int write_u64_le(FILE *out, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        if (fputc((int)(value & 0xffu), out) == EOF) return 0;
        value >>= 8;
    }
    return 1;
}

static int read_u64_le(FILE *in, uint64_t *value) {
    *value = 0;
    for (int i = 0; i < 8; i++) {
        int byte = fgetc(in);
        if (byte == EOF) return 0;
        *value |= (uint64_t)(unsigned char)byte << (8 * i);
    }
    return 1;
}

static int read_frequencies(FILE *in, uint64_t frequency[SYMBOLS], uint64_t *total) {
    unsigned char magic[4];
    if (fread(magic, 1, sizeof(magic), in) != sizeof(magic) ||
        memcmp(magic, MAGIC, sizeof(magic)) != 0) return 0;
    *total = 0;
    for (int i = 0; i < SYMBOLS; i++) {
        if (!read_u64_le(in, &frequency[i])) return 0;
        if (UINT64_MAX - *total < frequency[i]) return 0;
        *total += frequency[i];
    }
    return 1;
}

static int collect_frequencies(FILE *in, uint64_t frequency[SYMBOLS]) {
    memset(frequency, 0, sizeof(uint64_t) * SYMBOLS);
    int byte;
    while ((byte = fgetc(in)) != EOF) {
        if (frequency[(unsigned char)byte] == UINT64_MAX) return 0;
        frequency[(unsigned char)byte]++;
    }
    return !ferror(in);
}

/* Return 0 if an error occurs, 1 otherwise. */
static int write_packed_body(FILE *in, FILE *out, const Code codes[SYMBOLS]) {
    unsigned char packed = 0;
    int used = 0;
    int byte;
    while ((byte = fgetc(in)) != EOF) {
        const Code *code = &codes[(unsigned char)byte];
        for (int i = 0; i < code->length; i++) {
            packed = (unsigned char)((packed << 1) | code->bits[i]);
            if (++used == 8) {
                if (fputc(packed, out) == EOF) return 0;
                packed = 0;
                used = 0;
            }
        }
    }
    if (ferror(in)) return 0;
    if (used) {
        packed = (unsigned char)(packed << (8 - used)); /* zero-pad last byte */
        if (fputc(packed, out) == EOF) return 0;
    }
    return 1;
}

int compress_file(const char *input_path, const char *output_path) {
    if (strcmp(input_path, output_path) == 0) {
        fprintf(stderr, "Input and output must be different files.\n");
        return 1;
    }
    FILE *in = fopen(input_path, "rb");
    if (!in) { perror("Open input"); return 1; }
    uint64_t frequency[SYMBOLS];
    if (!collect_frequencies(in, frequency)) {
        fprintf(stderr, "Failed to read input.\n");
        fclose(in); return 1;
    }
    int ok;
    Node *root = build_tree(frequency, &ok);
    if (!ok) { fprintf(stderr, "Failed to build Huffman tree.\n"); fclose(in); return 1; }
    Code codes[SYMBOLS] = {0};
    unsigned char path[SYMBOLS] = {0};
    make_codes(root, codes, path, 0);

    FILE *out = fopen(output_path, "wb");
    if (!out) { perror("Open output"); free_tree(root); fclose(in); return 1; }
    ok = fwrite(MAGIC, 1, sizeof(MAGIC), out) == sizeof(MAGIC);
    for (int i = 0; ok && i < SYMBOLS; i++) ok = write_u64_le(out, frequency[i]);
    if (ok && fseek(in, 0, SEEK_SET) != 0) ok = 0;
    if (ok) ok = write_packed_body(in, out, codes);
    if (fclose(out) != 0) ok = 0;
    fclose(in);
    free_tree(root);
    if (!ok) {
        remove(output_path);
        fprintf(stderr, "Failed to write compressed data.\n");
        return 1;
    }
    printf("Compressed: %s -> %s\n", input_path, output_path);
    return 0;
}

static int decode_body(FILE *in, FILE *out, const Node *root, uint64_t total) {
    if (!total) return fgetc(in) == EOF && !ferror(in);
    if (is_leaf(root)) {
        for (uint64_t i = 0; i < total; i++) {
            if (fputc(root->symbol, out) == EOF) return 0;
        }
        return fgetc(in) == EOF && !ferror(in);
    }
    const Node *current = root;
    uint64_t written = 0;
    int byte;
    while (written < total && (byte = fgetc(in)) != EOF) {
        for (int bit = 7; bit >= 0; bit--) {
            current = ((byte >> bit) & 1) ? current->right : current->left;
            if (!current) return 0;
            if (is_leaf(current)) {
                if (fputc(current->symbol, out) == EOF) return 0;
                written++;
                current = root;
                if (written == total) {
                    /* Remaining bits in the current byte must be padding zeros. */
                    if ((byte & ((1 << bit) - 1)) != 0) return 0;
                    return fgetc(in) == EOF && !ferror(in);
                }
            }
        }
    }
    return 0; /* truncated file or incomplete final code */
}

int decompress_file(const char *input_path, const char *output_path) {
    if (strcmp(input_path, output_path) == 0) {
        fprintf(stderr, "Input and output must be different files.\n");
        return 1;
    }
    FILE *in = fopen(input_path, "rb");
    if (!in) { perror("Open input"); return 1; }
    uint64_t frequency[SYMBOLS], total;
    if (!read_frequencies(in, frequency, &total)) {
        fprintf(stderr, "Invalid or truncated HUF2 header.\n");
        fclose(in); return 1;
    }
    int ok;
    Node *root = build_tree(frequency, &ok);
    if (!ok) { fprintf(stderr, "Invalid Huffman tree.\n"); fclose(in); return 1; }
    FILE *out = fopen(output_path, "wb");
    if (!out) { perror("Open output"); free_tree(root); fclose(in); return 1; }
    ok = decode_body(in, out, root, total);
    if (fclose(out) != 0) ok = 0;
    fclose(in);
    free_tree(root);
    if (!ok) {
        remove(output_path);
        fprintf(stderr, "Corrupted or truncated compressed file.\n");
        return 1;
    }
    printf("Decompressed: %s -> %s\n", input_path, output_path);
    return 0;
}
