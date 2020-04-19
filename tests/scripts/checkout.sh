#!/bin/bash

# create random project/files on the server
cd tests_out/server
mkdir trainReservation
cd trainReservation
mkdir part1
mkdir part2
cd part1
mkdir part1_1
cd ../..
echo "aaaaaaaa" > trainReservation/file1
echo "bbbbbbbb" > trainReservation/file2
echo "cccccccc" > trainReservation/file3
echo "dddddddd" > trainReservation/part1/file1
echo "eeeeeeee" > trainReservation/part1/file2
echo "ffffffff" > trainReservation/part2/file1
echo "gggggggg" > trainReservation/part2/file2
echo "hhhhhhhh" > trainReservation/part1/part1_1/file1

# start server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000

# test checkout
../../bin/WTF checkout trainReservation

# test checkout error cases
../../bin/WTF checkout nonexistentProj
../../bin/WTF checkout trainReservation

# kill server
kill -INT $pid &2>/dev/null
wait $pid 2>/dev/null

diff -qr trainReservation ../server/trainReservation
