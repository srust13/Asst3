#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
result="$(../../bin/WTF history huffman_dir)"
expected='Server connected

1
A C84F8A070A28B21C1D84F6DDED7EB103 0 huffman_dir/file1
A 1F2252AC5146C4C0EECE85C4617C5D4F 0 huffman_dir/file3
2
M F5AC8127B3B6B85CDC13F237C6005D80 1 huffman_dir/file1
D 1F2252AC5146C4C0EECE85C4617C5D4F 0 huffman_dir/file3
A F9B11CF2A505297A82D209FB28B62FE2 0 huffman_dir/file2
3
A 7A9993E80A235A3632DEEC9A6ED2EDA1 0 huffman_dir/file3
4
D F5AC8127B3B6B85CDC13F237C6005D80 1 huffman_dir/file1
5
M 1FFB2614C326255B00E3D94DDED89DAF 1 huffman_dir/file2

Command completed successfully'

# kill server
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

[[ "$result" == "$expected" ]]
