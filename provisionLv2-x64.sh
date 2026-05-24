#!/bin/bash
TOOBAMP_DEB=$(ls ../ToobAmp/build/toobamp_*_amd64.deb | sort -V | tail -n1)
echo Provisioning $TOOBAMP_DEB
rm ./lv2/x86_64/*.deb
cp $TOOBAMP_DEB ./lv2/x86_64/