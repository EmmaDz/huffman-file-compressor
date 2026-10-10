#include <string.h>
#include "huffman.h"

static void print_usage(const char* prog) {
    fprintf(stderr,
        "Usage:\n"
        "  %s compress <input.txt> <output.huff>\n"
        "  %s decompress <input.huff> <output.txt>\n",
        prog, prog
    );
}

int main(int argc, char** argv) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char* mode = argv[1];
    const char* in   = argv[2];
    const char* out  = argv[3];

    if (strcmp(mode, "compress") == 0) {
        return compress_file(in, out);
    } else if (strcmp(mode, "decompress") == 0) {
        return decompress_file(in, out);
    } else {
        print_usage(argv[0]);
        return 1;
    }
}
