#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000
../../bin/WTF create huffman_dir

# kill server
sleep .1
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

# verify results
expected="0 huffman_dir"
given="$(cat huffman_dir/.Manifest)"
[[ "$expected" == "$given" ]]
