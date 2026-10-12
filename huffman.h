#ifndef HUFFMAN_H
#define HUFFMAN_H

/* Exit status: 0 = success, 1 = failure. */
#include <stdio.h>
int compress_file(const char *input_path, const char *output_path);
int decompress_file(const char *input_path, const char *output_path);

#endif
