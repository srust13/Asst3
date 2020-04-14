#!/bin/bash

# start server
cd test_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000
../../bin/WTF create huffman_dir

# kill server
kill -9 $pid &2>/dev/null
