#include "huffman.h"
#include "huffman_tree.h"
#include "canonical.h"
#include "bitio.h"
#include "huffman_header.h"

// Requires zero-initialized frequencies and total_symbols.
static int count_frequencies(FILE *input, uint64_t frequencies[SYMBOL_COUNT],
                             uint32_t *total_symbols) {
    int symbol;

    // Read bytes until EOF, counting each symbol and the total input size.
    while ((symbol = fgetc(input)) != EOF) {
        if (*total_symbols == UINT32_MAX) {
            return EXIT_FAILURE;
        }
        ++frequencies[symbol];
        ++(*total_symbols);
    }

    if (ferror(input)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Requires zero-initialized code_lengths for symbols absent from the tree.
static int prepare_codebook(const uint64_t frequencies[SYMBOL_COUNT],
                            uint8_t code_lengths[SYMBOL_COUNT],
                            huffman_code codebook[SYMBOL_COUNT]) {
    // Build a min heap and merge the two smallest nodes until one tree remains.
    huffman_node *root = NULL;
    int status = build_huffman_tree(frequencies, &root);
    if (status != EXIT_SUCCESS) {
        return status;
    }

    // Build lengths from the tree, indexed by symbol.
    build_code_lengths(root, code_lengths, 0);

    // Release the tree once its code lengths have been extracted.
    release_tree(root);

    // Apply the code length limit before building canonical codes.
    status = limit_code_lengths(frequencies, code_lengths);
    if (status != EXIT_SUCCESS) {
        return status;
    }

    // Check that the final lengths form a valid, supported prefix code.
    size_t codes_per_length[MAX_CODE_LEN + 1];
    status = validate_code_lengths(code_lengths, codes_per_length);
    if (status != EXIT_SUCCESS) {
        return status;
    }

    // Build a sorted array of symbol, length pairs.
    symbol_length_pair sorted_symbols[SYMBOL_COUNT];
    size_t code_count = build_sorted_symbol_length_pairs(sorted_symbols, code_lengths);

    // Build canonical codes from the sorted symbol lengths.
    build_canonical_codes(codebook, sorted_symbols, code_count);

    return EXIT_SUCCESS;
}

static int encode_payload(FILE *input, FILE *output,
                          const huffman_code codebook[SYMBOL_COUNT],
                          uint32_t total_symbols) {
    // The bit writer packs variable-length codes into output bytes.
    bit_writer writer;
    bit_writer_init(&writer, output);

    // Read each symbol again and write its translated canonical code.
    for (uint32_t index = 0; index < total_symbols; ++index) {
        int symbol = fgetc(input);
        if (symbol == EOF || codebook[symbol].length == 0) {
            return EXIT_FAILURE;
        }

        huffman_code code = codebook[symbol];
        if (bit_writer_write_bits(&writer, code.bits, code.length) == EOF) {
            return EXIT_FAILURE;
        }
    }

    // Write any remaining bits with zero padding, then flush the output stream.
    if (bit_writer_flush(&writer) == EOF || fflush(output) == EOF) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int huffman_encode(FILE *input, FILE *output) {
    if (input == NULL || output == NULL || input == output) {
        return EXIT_FAILURE;
    }

    // Save the starting position so the input can be read twice.
    fpos_t start_position;
    if (fgetpos(input, &start_position) != 0 || fsetpos(input, &start_position) != 0) {
        return EXIT_FAILURE;
    }

    uint64_t frequencies[SYMBOL_COUNT] = {0};
    uint32_t total_symbols = 0;
    int status = count_frequencies(input, frequencies, &total_symbols);
    if (status != EXIT_SUCCESS) {
        return status;
    }

    uint8_t code_lengths[SYMBOL_COUNT] = {0};
    huffman_code codebook[SYMBOL_COUNT];
    status = prepare_codebook(frequencies, code_lengths, codebook);
    if (status != EXIT_SUCCESS) {
        return status;
    }

    // Reset the input stream to where frequency counting began.
    if (fsetpos(input, &start_position) != 0) {
        return EXIT_FAILURE;
    }

    if (write_header(output, code_lengths, total_symbols) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return encode_payload(input, output, codebook, total_symbols);
}
