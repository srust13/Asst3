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
kill -INT $pid &2>/dev/null
wait $pid 2>/dev/null

exit 0
