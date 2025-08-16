#!/usr/bin/bash
# Build NetworkManager dbus proxies.
# This script generates the necessary code for the D-Bus proxies used by the NetworkManager.
# It is not performed as part of the build, since this is an infrequently performed operation
# that uses tools built as part of the libsdbus-c++ library.

# Relative path to XML2CPP tool from sdbus-cpp module.
# Assumes at least a partial build.
XML2CPP=../build/modules/sdbus-cpp/tools/sdbus-c++-xml2cpp

set -e

SOURCE_DIR=./src/include/dbus/xml
DEST_DIR=./src/include/dbus

for file in $DEST_DIR/*.hpp; do
    if [ -f "$file" ]; then
        filename=$(basename "$file" .hpp)
        echo $XML2CPP $SOURCE_DIR/${filename}.xml --proxy="$DEST_DIR/${filename}.hpp"        
        $XML2CPP $SOURCE_DIR/${filename}.xml --proxy="$DEST_DIR/${filename}.hpp"
    fi
done