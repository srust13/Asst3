#!/bin/bash

# create random files
cd tests_out/client/huffman_dir
echo "abcdefgh" > file1
echo "ijklmnop" > file2
echo "qrstuvwx" > file3

# test adds
../../../bin/WTF add huffman_dir file1
../../../bin/WTF add huffman_dir file2
../../../bin/WTF add huffman_dir file3

# remove one file
rm file2
../../../bin/WTF remove huffman_dir file2
