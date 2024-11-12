#!/usr/bin/bash

rm -rf build/lv2_x64
mkdir build/lv2_x64

mkdir build/lv2_x64/pkg
dpkg-deb -x lv2/x86_64/toobamp_*_amd64.deb  build/lv2_x64/pkg