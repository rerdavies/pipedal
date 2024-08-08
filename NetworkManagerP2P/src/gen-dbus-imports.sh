#!/bin/bash
rm -rf dbus/hpp
# dbus-send --system --type=method_call --print-reply --dest=fi.w1.wpa_supplicant1 /fi/w1/wpa_supplicant1 org.freedesktop.DBus.Introspectable.Introspect
mkdir dbus/hpp
for  filename in dbus/xml/*.xml; do
    echo $(basename "$filename" .xml)
    sdbus-c++-xml2cpp "$filename" --proxy=dbus/hpp/$(basename "$filename" .xml).hpp

done
