#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF push huffman_dir
echo "abcd" > huffman_dir/file1
echo "somethinnew" > huffman_dir/file2
../../bin/WTF add huffman_dir huffman_dir/file2
../../bin/WTF remove huffman_dir huffman_dir/file3
../../bin/WTF commit huffman_dir
../../bin/WTF push huffman_dir
rm huffman_dir/file3

# kill server
kill -INT $pid &2>/dev/null
wait $pid 2>/dev/null

result="$(cat huffman_dir/.Manifest)"
expected='2 huffman_dir
- F5AC8127B3B6B85CDC13F237C6005D80 1 huffman_dir/file1
- F9B11CF2A505297A82D209FB28B62FE2 0 huffman_dir/file2'
[[ "$result" == "$expected" ]] && diff -qr huffman_dir ../server/huffman_dir
