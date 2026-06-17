#!/usr/bin/bash

rm -rf build/lv2_x64
mkdir -p build/lv2_x64/pkg

# Extract .deb using ar + tar (portable, no dpkg-deb needed)
cd build/lv2_x64
ar x ../../lv2/x86_64/toobamp_*_amd64.deb
if [ -f data.tar.zst ]; then
    tar --zstd -xf data.tar.zst -C pkg
elif [ -f data.tar.xz ]; then
    tar -xJf data.tar.xz -C pkg
elif [ -f data.tar.gz ]; then
    tar -xzf data.tar.gz -C pkg
else
    echo "Error: cannot find data.tar.* in the .deb package"
    exit 1
fi
rm -f control.tar.* data.tar.* debian-binary
cd "$OLDPWD"