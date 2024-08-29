#!/bin/bash

#sign a package with rerdavies@gmail.com's private key.
cd build
for filename in *.deb; do
   echo gpg --armor --output "$filename".asc -b "$filename"

   gpg --armor --output "$filename".asc -b "$filename"
done
cd ..