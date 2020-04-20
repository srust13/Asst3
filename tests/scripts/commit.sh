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

diff -qr huffman_dir/.Commit ../server/huffman_dir/.Commit
