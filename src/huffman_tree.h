#ifndef HUFFMAN_TREE_H
#define HUFFMAN_TREE_H

#include "canonical.h"

typedef struct huffman_node {
    uint64_t frequency;
    struct huffman_node *left;
    struct huffman_node *right;
    uint8_t symbol;
} huffman_node;

int build_huffman_tree(const uint64_t *frequencies, huffman_node **root);
void release_tree(huffman_node *root);
void build_code_lengths(const huffman_node *root, uint8_t *code_lengths, unsigned depth);

#endif
