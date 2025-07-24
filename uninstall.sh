#!/usr/bin/sudo /usr/bin/bash
# CMake doesn't have an uninstall option, so remove files manually.
export PREFIX=/usr

rm $PREFIX/bin/pipedal*
rm $PREFIX/sbin/pipedal*
rm -rf /etc/pipedal/
rm -rf /var/pipedal/
rm -rf /usr/lib/lv2/ToobAmp.lv2/

