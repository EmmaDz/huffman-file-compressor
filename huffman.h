#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    unsigned char ch;
    int freq;
    struct Node* left;
    struct Node* right;
} Node;

typedef struct {
    char* bits;
    int len;
} Code;

int  count_frequency(const char* input_path, int freq[256]);
Node* build_huffman_tree(const int freq[256]);

void build_code_table(Node* root, Code table[256]);

int compress_file(const char* input_path, const char* output_path);
int decompress_file(const char* input_path, const char* output_path);

void free_tree(Node* root);
int  is_leaf(const Node* n);

void free_code_table(Code table[256]);

#endif
