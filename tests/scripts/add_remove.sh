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

# verify results
given="$(cat huffman_dir/.Manifest)"
expected='0 huffman_dir
A C84F8A070A28B21C1D84F6DDED7EB103 0 huffman_dir/file1
D 6DAD2BC7378B4B6DC929B2ECCD9DF5C8 0 huffman_dir/file2
A 1F2252AC5146C4C0EECE85C4617C5D4F 0 huffman_dir/file3'
[[ "$given" == "$expected" ]]
