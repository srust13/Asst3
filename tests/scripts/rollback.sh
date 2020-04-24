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
expected_manifest_contents="$(cat ../server/rback/.Manifest)"

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

# kill server
sleep .1
kill -INT $pid 2>/dev/null
wait $pid 2>/dev/null

given_backups_rback="$(ls -aR ../server/backups/rback | sort)"
expected_backups_rback='../server/backups/rback:
.
somedir
.Manifest_1
file2_0
file1_0
.Manifest_0
..

../server/backups/rback/somedir:
..
.
file3_0'
expected_backups_rback=$(echo "$expected_backups_rback" | sort)

given_server_rback="$(ls -aR ../server/rback | sort)"
expected_server_rback='../server/rback:
..
.
somedir
file1
file2
.Manifest

../server/rback/somedir:
.
..
file3'
expected_server_rback=$(echo "$expected_server_rback" | sort)

given_manifest_contents="$(cat ../server/rback/.Manifest)"
[[ "$given_server_rback" == "$expected_server_rback" ]] && [[ "$given_backups_rback" == "$expected_backups_rback" ]] && [[ "$expected_manifest_contents" == "$given_manifest_contents" ]]
