# Canonical Huffman Coding in C

A command-line program for compressing and decompressing files using canonical, length limited, Huffman coding. For a more in-depth breakdown of what this project implements, compression diagnostics, as well as general incites into larger field of information theory, check out my blog entry on this project: (url here later)

As a brief overview: Huffman coding is a lossless compression algorithm that assigns shorter codes to more frequent bytes. These codes can be written to a separate compressed file, along with a header containing a codebook that allows the original data to be reconstructed. Canonical Huffman coding improves this representation by allowing the decoder to reconstruct the codes from their lengths and symbol order, so the compressed file only needs to store the code lengths rather than the full tree.

## Build and run

Requires a C17 compiler and GNU Make. Run these commands from the project directory:

```sh
make

# Compress a file
./huffman -e input.txt compressed.huf

# Decompress it
./huffman -d compressed.huf restored.txt

# Verify the restored file matches the original
cmp input.txt restored.txt
```

Both text and binary files are supported. Use separate input and output files; an existing output file is overwritten.

## How it works

The encoder processes data in these stages:

```text
Input bytes
    - byte frequencies
    - Huffman tree
    - code lengths indexed by symbol
    - symbols sorted by length, then symbol value
    - canonical codebook
    - header and encoded bits
```

A min-heap repeatedly selects the two least frequent nodes to build the tree. Each leaf's depth determines its code length. The encoder then assigns canonical codes and looks up each input byte in the resulting codebook.

The decoder reads the code lengths from the header and reconstructs the canonical ordering. It reads payload bits until a code matches, then writes the corresponding byte. It does not need to rebuild the original tree.

## Project structure

| File | Purpose |
| --- | --- |
| `src/main.c` | Command-line arguments and file handling |
| `src/huffman_encode.c` | Compression pipeline |
| `src/huffman_decode.c` | Decompression pipeline |
| `src/huffman_header.c` | Binary header reading and writing |
| `src/huffman_tree.c` | Tree construction and code lengths |
| `src/minheap.c` | Priority queue used during tree construction |
| `src/canonical.c` | Code-length validation, symbol sorting, and canonical codes |
| `src/bitio.c` | Buffered bit reading and writing |
| `tests/` | Assert-based tests for the core modules |

## File format

The program uses a custom binary format:

| Field | Size | Description |
| --- | --- | --- |
| Code lengths | 256 bytes | One length per possible byte value; zero means unused |
| Original size | 4 bytes | Number of original bytes, stored in little-endian order |
| Payload | Variable | Encoded bits with zero padding in the final byte |

Each code is emitted most significant bit first. Within an output byte, those bits occupy positions 0 through 7 in that order.

## Tests

```sh
make test
```

Tests cover bit ordering and flushing, canonical code generation and validation, heap operations, tree construction, and encode/decode round trips. They also check basic invalid inputs, empty files, and single-symbol data.

Additional build commands:

```sh
make debug  # Build the program with address and undefined-behavior sanitizers
make clean  # Remove the program and test executables
```

The debug target requires a compiler that supports the sanitizer flags in the Makefile.

## Current limitations

- Code lengths are capped at 15 bits.
- The encoder reads the input twice, so it requires seekable input.
- The four-byte size field supports at most 4,294,967,295 original bytes.
- The fixed 260-byte header can make small files larger after compression. Compression savings depend on the input's byte distribution.
- The format has no checksum; structural validation does not detect every possible data corruption.
