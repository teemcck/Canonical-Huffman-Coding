#include <assert.h>
#include <stdio.h>
#include "huffman_tree.h"

// ============================================
// Test: Empty tree and a single symbol
// ============================================
void test_empty_and_single(void) {
    uint64_t frequencies[SYMBOL_COUNT] = {0};
    uint8_t code_lengths[SYMBOL_COUNT] = {0};
    huffman_node *root = NULL;
    assert(build_huffman_tree(frequencies, &root) == EXIT_SUCCESS);
    assert(root == NULL);
    build_code_lengths(root, code_lengths, 0);
    assert(code_lengths['A'] == 0);
    release_tree(root);

    frequencies['A'] = 5;
    assert(build_huffman_tree(frequencies, &root) == EXIT_SUCCESS);
    assert(root != NULL);
    assert(root->symbol == 'A' && root->frequency == 5);
    assert(root->left == NULL && root->right == NULL);
    build_code_lengths(root, code_lengths, 0);
    assert(code_lengths['A'] == 1);
    release_tree(root);
}


// ============================================
// Test: Build a small tree and its code lengths
// ============================================
void test_small_tree(void) {
    uint64_t frequencies[SYMBOL_COUNT] = {0};
    uint8_t code_lengths[SYMBOL_COUNT] = {0};
    frequencies['A'] = 5;
    frequencies['B'] = 2;
    frequencies['C'] = 1;
    huffman_node *root = NULL;
    assert(build_huffman_tree(frequencies, &root) == EXIT_SUCCESS);
    assert(root != NULL && root->frequency == 8);
    build_code_lengths(root, code_lengths, 0);
    assert(code_lengths['A'] == 1);
    assert(code_lengths['B'] == 2 && code_lengths['C'] == 2);
    assert(code_lengths['D'] == 0);
    release_tree(root);
}


// ============================================
// Main test runner
// ============================================
int main(void) {
    printf("Running huffman_tree tests...\n");

    test_empty_and_single();
    test_small_tree();

    printf("All tests passed!\n");
    return 0;
}
