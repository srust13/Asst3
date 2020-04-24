#!/bin/bash

# start server
cd tests_out/server
../../bin/WTFserver 5000 &
pid=$!

# create new project with some files
cd ../client
../../bin/WTF configure localhost 5000
../../bin/WTF create rback

# push 1
echo "sometext" > rback/file1
echo "moretext" > rback/file2
mkdir -p rback/somedir/anotherdir
echo "somethingmore" > rback/somedir/file3
../../bin/WTF add rback rback/file1
../../bin/WTF add rback rback/file2
../../bin/WTF add rback rback/somedir/file3
../../bin/WTF commit rback
../../bin/WTF push rback

# push 2
echo "evenmore" > rback/somedir/anotherdir/file4
../../bin/WTF add rback rback/somedir/anotherdir/file4
../../bin/WTF commit rback
../../bin/WTF push rback

# push 3
echo "somethingdifferent" > rback/somedir/anotherdir/file4
../../bin/WTF add rback rback/somedir/anotherdir/file4
../../bin/WTF commit rback
../../bin/WTF push rback

# rollback
../../bin/WTF rollback rback 1
rm -rf rback
../../bin/WTF checkout rback

ls -alh ../server/backups/rback/somedir

# kill server
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null
