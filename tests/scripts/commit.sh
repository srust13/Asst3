#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000
../../bin/WTF commit huffman_dir

# kill server
kill -INT $pid &2>/dev/null
wait $pid 2>/dev/null

result="$(cat huffman_dir/.Commit)"
expected='A C84F8A070A28B21C1D84F6DDED7EB103 0 huffman_dir/file1
A 1F2252AC5146C4C0EECE85C4617C5D4F 0 huffman_dir/file3'

diff -qr huffman_dir/.Commit ../server/huffman_dir/.Commit* && [[ "$expected" == "$result" ]]
