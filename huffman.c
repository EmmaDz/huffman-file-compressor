#include <string.h>
#include "huffman.h"

static Node* new_node(unsigned char ch, int freq, Node* left, Node* right) {
    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) return NULL;
    n->ch = ch;
    n->freq = freq;
    n->left = left;
    n->right = right;
    return n;
}

static int pick_min_index(Node** arr, int n) {
    int idx = -1;
    for (int i = 0; i < n; i++) {
        if (!arr[i]) continue;
        if (idx == -1 || arr[i]->freq < arr[idx]->freq) {
            idx = i;
        }
    }
    return idx;
}


int count_frequency(const char* input_path, int freq[256]) {
    // init freq array
    for (int i = 0; i < 256; i++) freq[i] = 0;

    FILE* f = fopen(input_path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open input file: %s\n", input_path);
        return -1;
    }

    int c;
    int total = 0;
    while ((c = fgetc(f)) != EOF) {
        freq[(unsigned char)c]++;
        total++;
    }

    fclose(f);
    return total; // number of bytes read
}


Node* build_huffman_tree(const int freq[256]) {
    // collect initial leaf nodes
    Node* nodes[512]; // safe upper bound: <= 2*leaves
    int count = 0;
    for (int i = 0; i < 512; i++) nodes[i] = NULL;

    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            Node* leaf = new_node((unsigned char)i, freq[i], NULL, NULL);
            if (!leaf) {
                fprintf(stderr, "Error: malloc failed while creating leaf\n");
                // best effort cleanup
                for (int k = 0; k < count; k++) free_tree(nodes[k]);
                return NULL;
            }
            nodes[count++] = leaf;
        }
    }

    if (count == 0) return NULL;      // empty input
    if (count == 1) return nodes[0];  // only one unique char

    int active = count;

    // repeatedly merge two smallest
    while (1) {
        int i1 = pick_min_index(nodes, active);
        Node* a = nodes[i1];
        nodes[i1] = NULL;

        int i2 = pick_min_index(nodes, active);
        Node* b = nodes[i2];
        nodes[i2] = NULL;

        Node* parent = new_node(0, a->freq + b->freq, a, b);
        if (!parent) {
            fprintf(stderr, "Error: malloc failed while merging nodes\n");
            // cleanup remaining nodes + subtrees
            for (int i = 0; i < active; i++) {
                if (nodes[i]) free_tree(nodes[i]);
            }
            free_tree(a);
            free_tree(b);
            return NULL;
        }

        // put parent into first empty slot
        int placed = 0;
        for (int i = 0; i < active; i++) {
            if (!nodes[i]) {
                nodes[i] = parent;
                placed = 1;
                break;
            }
        }
        if (!placed) {
            // should not happen, but just in case
            nodes[active++] = parent;
        }

        // check how many non-null nodes remain
        int remaining = 0;
        int last_idx = -1;
        for (int i = 0; i < active; i++) {
            if (nodes[i]) {
                remaining++;
                last_idx = i;
            }
        }

        if (remaining == 1) {
            return nodes[last_idx]; // root
        }
    }
}


static void dfs_build_codes(Node* node, Code table[256], char* path, int depth) {
    if (!node) return;

    if (is_leaf(node)) {
        // Special case: only one unique char -> give it "0"
        int len = (depth == 0) ? 1 : depth;

        char* s = (char*)malloc((size_t)len + 1);
        if (!s) {
            fprintf(stderr, "Error: malloc failed while creating code\n");
            return;
        }

        if (depth == 0) {
            s[0] = '0';
        } else {
            for (int i = 0; i < depth; i++) s[i] = path[i];
        }
        s[len] = '\0';

        table[node->ch].bits = s;
        table[node->ch].len = len;
        return;
    }

    // left -> '0'
    path[depth] = '0';
    dfs_build_codes(node->left, table, path, depth + 1);

    // right -> '1'
    path[depth] = '1';
    dfs_build_codes(node->right, table, path, depth + 1);
}

void build_code_table(Node* root, Code table[256]) {
    for (int i = 0; i < 256; i++) {
        table[i].bits = NULL;
        table[i].len = 0;
    }
    char path[512]; // enough for our one-day version
    dfs_build_codes(root, table, path, 0);
}

static int write_header(FILE* out, const int freq[256]) {
    for (int i = 0; i < 256; i++) {
        if (fprintf(out, "%d\n", freq[i]) < 0) return 0;
    }
    if (fprintf(out, "--END--\n") < 0) return 0;
    return 1;
}

static int read_header(FILE* in, int freq[256]) {
    // read 256 lines of integers
    for (int i = 0; i < 256; i++) {
        if (fscanf(in, "%d", &freq[i]) != 1) {
            fprintf(stderr, "Error: failed to read freq[%d] from header\n", i);
            return 0;
        }
        // consume the rest of the line (including newline)
        int ch;
        while ((ch = fgetc(in)) != EOF && ch != '\n') { }
    }

    // read the "--END--" line
    char marker[32];
    if (!fgets(marker, (int)sizeof(marker), in)) {
        fprintf(stderr, "Error: failed to read header end marker\n");
        return 0;
    }

    if (strncmp(marker, "--END--", 7) != 0) {
        fprintf(stderr, "Error: invalid header end marker: %s\n", marker);
        return 0;
    }

    return 1;
}

static int encode_and_write_body(const char* input_path, FILE* out, Code table[256]) {
    FILE* in = fopen(input_path, "rb");
    if (!in) {
        fprintf(stderr, "Error: cannot reopen input file: %s\n", input_path);
        return 0;
    }

    int c;
    while ((c = fgetc(in)) != EOF) {
        Code code = table[(unsigned char)c];
        if (!code.bits) {
            fprintf(stderr, "Error: missing code for byte %d\n", c);
            fclose(in);
            return 0;
        }
        // write the '0'/'1' characters
        if (fputs(code.bits, out) == EOF) {
            fprintf(stderr, "Error: failed writing compressed body\n");
            fclose(in);
            return 0;
        }
    }

    fclose(in);
    return 1;
}

int compress_file(const char* input_path, const char* output_path) {
    int freq[256];
    int total = count_frequency(input_path, freq);
    if (total < 0) return 1;

    Node* root = build_huffman_tree(freq);
    if (!root) {
        fprintf(stderr, "Error: failed to build Huffman tree\n");
        return 1;
    }

    Code table[256];
    build_code_table(root, table);

    FILE* out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "Error: cannot open output file: %s\n", output_path);
        free_code_table(table);
        free_tree(root);
        return 1;
    }

    // header: frequencies
    if (!write_header(out, freq)) {
        fprintf(stderr, "Error: failed writing header\n");
        fclose(out);
        free_code_table(table);
        free_tree(root);
        return 1;
    }

    // body: encoded bits as '0'/'1'
    if (!encode_and_write_body(input_path, out, table)) {
        fprintf(stderr, "Error: failed writing body\n");
        fclose(out);
        free_code_table(table);
        free_tree(root);
        return 1;
    }

    fclose(out);

    // cleanup
    free_code_table(table);
    free_tree(root);

    fprintf(stderr, "Compressed OK: %s -> %s (input bytes=%d)\n", input_path, output_path, total);

    return 0;
}


int decompress_file(const char* input_path, const char* output_path) {
    FILE* in = fopen(input_path, "rb");
    if (!in) {
        fprintf(stderr, "Error: cannot open input file: %s\n", input_path);
        return 1;
    }

    int freq[256];
    if (!read_header(in, freq)) {
        fclose(in);
        return 1;
    }

    Node* root = build_huffman_tree(freq);
    if (!root) {
        fprintf(stderr, "Error: failed to rebuild Huffman tree\n");
        fclose(in);
        return 1;
    }

    FILE* out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "Error: cannot open output file: %s\n", output_path);
        free_tree(root);
        fclose(in);
        return 1;
    }

    // decode body
    Node* cur = root;
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if (ch == '0') {
            cur = cur->left;
        } else if (ch == '1') {
            cur = cur->right;
        } else {
            // ignore any whitespace/newlines (just in case)
            continue;
        }

        if (!cur) {
            fprintf(stderr, "Error: corrupted bitstream (hit NULL)\n");
            fclose(out);
            free_tree(root);
            fclose(in);
            return 1;
        }

        if (is_leaf(cur)) {
            fputc(cur->ch, out);
            cur = root;
        }
    }

    fclose(out);
    free_tree(root);
    fclose(in);

    fprintf(stderr, "Decompressed OK: %s -> %s\n", input_path, output_path);
    return 0;
}

int is_leaf(const Node* n) {
    return n && !n->left && !n->right;
}

void free_tree(Node* root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

void free_code_table(Code table[256]) {
    for (int i = 0; i < 256; i++) {
        free(table[i].bits);
        table[i].bits = NULL;
        table[i].len = 0;
    }
}

