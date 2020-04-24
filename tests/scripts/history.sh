#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client

result="$(../../bin/WTF history huffman_dir)"
expected=''

# kill server
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

[[ "$result" == "$expected" ]]

