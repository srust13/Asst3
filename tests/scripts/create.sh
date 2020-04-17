#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000
../../bin/WTF create huffman_dir

sleep .5

# kill server
kill -9 $pid &2>/dev/null
