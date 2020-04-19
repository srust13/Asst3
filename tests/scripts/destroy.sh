#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# start client
cd ../client
../../bin/WTF configure localhost 5000

# test destroy
../../bin/WTF destroy trainReservation

# test destroy error cases
../../bin/WTF destroy nonexistentProj && exit 1

# kill server
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

[ ! -d ../server/trainReservation ]
