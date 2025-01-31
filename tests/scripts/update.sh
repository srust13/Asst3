#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# create another client
cd ..
mkdir client2
cd client2
../../bin/WTF configure localhost 5000
../../bin/WTF checkout huffman_dir

# tests the "A" of update
echo "this is a new file" > huffman_dir/file3
../../bin/WTF add huffman_dir huffman_dir/file3
../../bin/WTF commit huffman_dir
../../bin/WTF push huffman_dir

# tests the "D" of update
../../bin/WTF remove huffman_dir huffman_dir/file1
../../bin/WTF commit huffman_dir
../../bin/WTF push huffman_dir

# tests the "M" of update
echo "adding line" >> huffman_dir/file2
../../bin/WTF commit huffman_dir
../../bin/WTF push huffman_dir

# tests the "C" of update
cd ../client
echo "i'm changing this file" >> huffman_dir/file2
../../bin/WTF update huffman_dir

# kill server
sleep .1
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

result_update="$(cat huffman_dir/.Update)"
expected_update='A 7A9993E80A235A3632DEEC9A6ED2EDA1 0 huffman_dir/file3
D F5AC8127B3B6B85CDC13F237C6005D80 1 huffman_dir/file1'

result_conflict="$(cat huffman_dir/.Conflict)"
expected_conflict='C F9B11CF2A505297A82D209FB28B62FE2 0 huffman_dir/file2'

result_manifest="$(cat huffman_dir/.Manifest)"
expected_manifest='2 huffman_dir
- F5AC8127B3B6B85CDC13F237C6005D80 1 huffman_dir/file1
- F9B11CF2A505297A82D209FB28B62FE2 0 huffman_dir/file2'


[[ "$result_update" == "$expected_update" ]] && [[ "$result_conflict" == "$expected_conflict" ]] &&
[[ "$result_manifest" == "$expected_manifest" ]]
