#include <assert.h>
#include <stdio.h>
#include "canonical.h"
#include "huffman_tree.h"

// ============================================
// Test: Sort by length, then symbol
// ============================================
void test_sorted_pairs(void) {
    uint8_t code_lengths[SYMBOL_COUNT] = {0};
    symbol_length_pair sorted_symbols[SYMBOL_COUNT];
    assert(build_sorted_symbol_length_pairs(sorted_symbols, code_lengths) == 0);
    code_lengths['C'] = 2;
    code_lengths['A'] = 2;
    code_lengths['B'] = 1;
    assert(build_sorted_symbol_length_pairs(sorted_symbols, code_lengths) == 3);
    assert(sorted_symbols[0].symbol == 'B' && sorted_symbols[0].length == 1);
    assert(sorted_symbols[1].symbol == 'A' && sorted_symbols[1].length == 2);
    assert(sorted_symbols[2].symbol == 'C' && sorted_symbols[2].length == 2);
}


// ============================================
// Test: Validate basic code lengths
// ============================================
void test_validate_lengths(void) {
    uint8_t code_lengths[SYMBOL_COUNT] = {0};
    size_t codes_per_length[MAX_CODE_LEN + 1];
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_SUCCESS);
    code_lengths['A'] = 1;
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_SUCCESS);
    code_lengths['B'] = 2;
    code_lengths['C'] = 2;
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_SUCCESS);
    assert(codes_per_length[1] == 1 && codes_per_length[2] == 2);

    code_lengths['C'] = 0; // Incomplete tree.
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_FAILURE);
    code_lengths['B'] = 1;
    code_lengths['C'] = 1; // Too many one-bit codes.
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_FAILURE);
    code_lengths['C'] = MAX_CODE_LEN + 1;
    assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_FAILURE);
}


// ============================================
// Test: Build known canonical codes
// ============================================
void test_build_codes(void) {
    symbol_length_pair sorted_symbols[] = {{'A', 1}, {'B', 2}, {'C', 2}};
    huffman_code codes[SYMBOL_COUNT];
    build_canonical_codes(codes, sorted_symbols, 3);
    assert(codes['A'].bits == 0 && codes['A'].length == 1);
    assert(codes['B'].bits == 2 && codes['B'].length == 2);
    assert(codes['C'].bits == 3 && codes['C'].length == 2);
    assert(codes['D'].length == 0);

    build_canonical_codes(codes, sorted_symbols, 0);
    assert(codes['A'].bits == 0 && codes['A'].length == 0);
}


// ============================================
// Test: Limit deep Huffman trees
// ============================================
void test_deep_tree_lengths(void) {
    const size_t symbol_counts[] = {17, 24, 40};
    for (size_t test = 0; test < sizeof(symbol_counts) / sizeof(symbol_counts[0]); ++test) {
        uint64_t frequencies[SYMBOL_COUNT] = {0};
        uint8_t code_lengths[SYMBOL_COUNT] = {0};
        uint64_t previous = 0, current = 1;

        // Fibonacci frequencies create deep trees; scramble symbol order so
        // assigning lengths by symbol value cannot pass the frequency checks.
        for (size_t rank = 0; rank < symbol_counts[test]; ++rank) {
            size_t symbol = (73 * rank + 19) % SYMBOL_COUNT;
            frequencies[symbol] = current;
            uint64_t next = previous + current;
            previous = current;
            current = next;
        }
        huffman_node *root = NULL;
        assert(build_huffman_tree(frequencies, &root) == EXIT_SUCCESS);
        build_code_lengths(root, code_lengths, 0);
        release_tree(root);

        unsigned deepest = 0;
        for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
            if (code_lengths[symbol] > deepest) {
                deepest = code_lengths[symbol];
            }
        }
        assert(deepest > MAX_CODE_LEN); // Ensure the repair path is exercised.
        assert(limit_code_lengths(frequencies, code_lengths) == EXIT_SUCCESS);

        uint32_t kraft_sum = 0;
        size_t present = 0;
        for (size_t symbol = 0; symbol < SYMBOL_COUNT; ++symbol) {
            if (frequencies[symbol] == 0) {
                assert(code_lengths[symbol] == 0);
                continue;
            }
            assert(code_lengths[symbol] > 0 && code_lengths[symbol] <= MAX_CODE_LEN);
            ++present;
            kraft_sum += 1u << (MAX_CODE_LEN - code_lengths[symbol]);
            for (size_t other = 0; other < SYMBOL_COUNT; ++other) {
                if (frequencies[symbol] > frequencies[other] && frequencies[other] != 0) {
                    assert(code_lengths[symbol] <= code_lengths[other]);
                }
            }
        }
        assert(present == symbol_counts[test]);
        assert(kraft_sum == (1u << MAX_CODE_LEN));
        size_t codes_per_length[MAX_CODE_LEN + 1];
        assert(validate_code_lengths(code_lengths, codes_per_length) == EXIT_SUCCESS);
    }
}


// ============================================
// Main test runner
// ============================================
int main(void) {
    printf("Running canonical tests...\n");

    test_sorted_pairs();
    test_validate_lengths();
    test_build_codes();
    test_deep_tree_lengths();

    printf("All tests passed!\n");
    return 0;
}
