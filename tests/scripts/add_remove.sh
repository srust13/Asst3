#!/bin/bash

# create random files
cd tests_out/client
echo "abcdefgh" > huffman_dir/file1
echo "ijklmnop" > huffman_dir/file2
echo "qrstuvwx" > huffman_dir/file3

# test adds and modifies
../../bin/WTF add huffman_dir huffman_dir/file1
../../bin/WTF add huffman_dir huffman_dir/file2
../../bin/WTF add huffman_dir huffman_dir/file3
../../bin/WTF add huffman_dir huffman_dir/file3

# remove one file
rm huffman_dir/file2
../../bin/WTF remove huffman_dir huffman_dir/file2
exit 0
