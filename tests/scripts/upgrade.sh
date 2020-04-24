#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client

# resolve .Conflict
echo "somethinnew" > huffman_dir/file2
../../bin/WTF update huffman_dir

# upgrade
../../bin/WTF upgrade huffman_dir

# kill server
kill -INT $pid &2>/dev/null
wait $pid 2>/dev/null

result="$(cat huffman_dir/.Manifest)"
expected='5 huffman_dir
- 1FFB2614C326255B00E3D94DDED89DAF 1 huffman_dir/file2
A 7A9993E80A235A3632DEEC9A6ED2EDA1 0 huffman_dir/file3'

[[ "$result" == "$expected" ]]
