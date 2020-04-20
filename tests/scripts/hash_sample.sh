#!/bin/bash

echo "c71b94505304dbad1526882c36fae444 huffman_dir/.Manifest" | md5sum -c --quiet - && \
	 ( diff -qr tests_out/client/huffman_dir tests_out/server/huffman_dir
