#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"

// Canonical Huffman Coding Encoding/Decoding Algorithm
// Author: Tristan McCaskill
// Date (M/D/Y): 07/16/26
// Reference: https://huffman-coding-online.vercel.app/
// Encoding time complexity: O(n + k log k), with code lengths capped at 15 bits.
// n = input bytes, k = distinct symbols (at most 256).

// Encoding goat does not affect size?

int main(int argc, char *argv[]) {
    if (argc != 4 || (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-d") != 0)) {
        fprintf(stderr, "Usage: %s -e/-d INPUT_FILE OUTPUT_FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Check identical filenames before opening the output, which clears its contents.
    // Different paths or links to the same file are not detected by this check.
    if (strcmp(argv[2], argv[3]) == 0) {
        fprintf(stderr, "Input and output filenames must be different\n");
        return EXIT_FAILURE;
    }

    // Open the input and output files in binary mode.
    FILE *input = fopen(argv[2], "rb");
    if (input == NULL) {
        perror("Opening input");
        return EXIT_FAILURE;
    }

    FILE *output = fopen(argv[3], "wb");
    if (output == NULL) {
        perror("Opening output");
        fclose(input);
        return EXIT_FAILURE;
    }

    int status;
    if (strcmp(argv[1], "-e") == 0) {
        status = huffman_encode(input, output);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "Encoding failed\n");
        }
    } else {
        status = huffman_decode(input, output);
        if (status != EXIT_SUCCESS) {
            fprintf(stderr, "Decoding failed\n");
        }
    }

    // Close both files and check for errors writing buffered output.
    if (fclose(input) == EOF) {
        perror("Closing input");
        status = EXIT_FAILURE;
    }
    if (fclose(output) == EOF) {
        perror("Closing output");
        status = EXIT_FAILURE;
    }

    return status;
}
