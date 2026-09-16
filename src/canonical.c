#include <stdlib.h>
#include "canonical.h"

// Sort shorter codes first; break ties using the symbol value.
static int compare_symbol_lengths(const void *left, const void *right) {
    const symbol_length_pair *first = left;
    const symbol_length_pair *second = right;

    if (first->length != second->length) {
        return (first->length > second->length) - (first->length < second->length);
    } 
    return (first->symbol > second->symbol) - (first->symbol < second->symbol);
}

size_t build_sorted_symbol_length_pairs(symbol_length_pair *sorted_symbols, const uint8_t *code_lengths) {
    size_t code_count = 0;

    // A zero length means the symbol does not occur in the code table.
    for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
        if (code_lengths[symbol] != 0) {
            sorted_symbols[code_count++] = (symbol_length_pair){(uint8_t)symbol, code_lengths[symbol]};
        }
    }
    qsort(sorted_symbols, code_count, sizeof(*sorted_symbols), compare_symbol_lengths);

    return code_count;
}

int validate_code_lengths(const uint8_t *code_lengths, size_t *codes_per_length) {
    for (size_t length = 0; length <= MAX_CODE_LEN; ++length) {
        codes_per_length[length] = 0;
    }

    size_t code_count = 0;

    for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
        if (code_lengths[symbol] > MAX_CODE_LEN) {
            return EXIT_FAILURE;
        }
        if (code_lengths[symbol] != 0) {
            ++codes_per_length[code_lengths[symbol]];
            ++code_count;
        }
    }

    // Each unused prefix has two children at the next tree level.
    // Assigning a code consumes a slot, so it cannot be reused as a prefix.
    size_t available = 1;
    for (size_t length = 1; length <= MAX_CODE_LEN; ++length) {
        available *= 2;
        if (codes_per_length[length] > available) {
            return EXIT_FAILURE;
        }
        available -= codes_per_length[length];
    }

    // Accept a full tree, or the special cases of empty input and one symbol.
    if (code_count == 0 || (code_count == 1 && codes_per_length[1] == 1) || available == 0) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

// Requires validated pairs sorted by length, then symbol.
void build_canonical_codes(huffman_code *codes, const symbol_length_pair *sorted_symbols, size_t code_count) {
    uint32_t current_code = 0;
    uint8_t previous_length = 0;

    for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
        codes[symbol].bits = 0;
        codes[symbol].length = 0;
    }

    for (size_t index = 0; index < code_count; ++index) {
        // Append zero bits when moving to a longer code length.
        current_code <<= sorted_symbols[index].length - previous_length;

        codes[sorted_symbols[index].symbol] = (huffman_code){(uint16_t)current_code, sorted_symbols[index].length};
        previous_length = sorted_symbols[index].length;
        ++current_code;
    }
}

static void assign_code_lengths(const uint64_t *frequencies,
                                const size_t *histogram,
                                uint8_t *code_lengths) {
    // Pass 3: Sort present symbols by decreasing frequency.
    uint8_t symbols[SYMBOL_COUNT];
    size_t count = 0;
    for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
        if (code_lengths[symbol] == 0) {
            continue;
        }
        size_t i = count++;
        while (i > 0 && frequencies[symbols[i - 1]] < frequencies[symbol]) {
            symbols[i] = symbols[i - 1];
            --i;
        }
        symbols[i] = (uint8_t)symbol;
    }

    // Copy histogram back into code_lengths, most frequent symbols first.
    size_t index = 0;
    for (unsigned length = 1; length <= MAX_CODE_LEN; ++length) {
        for (size_t i = 0; i < histogram[length]; ++i) {
            code_lengths[symbols[index++]] = (uint8_t)length;
        }
    }
}

// MiniZ's Length-Limiting Algorithm. Limits codes to MAX_CODE_LEN from SYMBOL_COUNT.
// Reference: https://github.com/richgel999/miniz/blob/master/miniz_tdef.c#L197
int limit_code_lengths(const uint64_t *frequencies, uint8_t *code_lengths) {
    // Build histogram: Stores the number of symbols that fall into buckets by bit-length.
    size_t histogram[SYMBOL_COUNT] = {0};

    for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
        unsigned length = code_lengths[symbol];
        if (length != 0) {
            ++histogram[length];
        }
    }

    // Pass 1: Brute force clamp (>MAX_LEN) codes to MAX_LEN.
    size_t codes_above_max = 0;
    for (size_t length = MAX_CODE_LEN + 1; length < SYMBOL_COUNT; ++length) {
        codes_above_max += histogram[length];
        histogram[length] = 0;
    }
    if (codes_above_max == 0) {
        return EXIT_SUCCESS;
    }
    histogram[MAX_CODE_LEN] += codes_above_max;

    // Pass 2: Repair Kraft sum by looping through:
    uint32_t kraft_sum = 0;
    for (unsigned length = 1; length <= MAX_CODE_LEN; ++length) {
        kraft_sum += (uint32_t)histogram[length] << (MAX_CODE_LEN - length);
    }
    // We use integer scaled values (by 2^MAX_CODE_LEN) for precise fractional numbers.
    uint32_t target = 1u << MAX_CODE_LEN;
    while (kraft_sum > target) {
        int length = MAX_CODE_LEN - 1;
        while (length > 0 && histogram[length] == 0) {
            --length;
        }
        if (length == 0 || histogram[MAX_CODE_LEN] == 0) {
            return EXIT_FAILURE;
        }

        --histogram[MAX_CODE_LEN];
        // Lift next non-zero code to free up more space.
        --histogram[length];
        histogram[length + 1] += 2;
        // Reduced the Kraft sum by 1.
        --kraft_sum;
    }

    assign_code_lengths(frequencies, histogram, code_lengths);

    return EXIT_SUCCESS;
}
