#!/bin/bash

# start server from client since client's
# manifest is more interesting
cd tests_out/client
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../server
../../bin/WTF configure localhost 5000

result="$(../../bin/WTF currentversion huffman_dir)"
expected='Server connected

----------------------------------------------
Version | Filename
----------------------------------------------
0 huffman_dir/file1
0 huffman_dir/file2
0 huffman_dir/file3

Client gracefully disconnected from server
Command completed successfully'

# kill server
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

# cleanup
rm .configure

# return diff
[[ "$result" == "$expected" ]]
