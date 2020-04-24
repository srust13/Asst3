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
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

exit 0
