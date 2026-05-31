#!/usr/bin/bash
export SRC_ZIP="V3 Factory Presets.piBank"
export DST_ZIP="Factory Presets.piBank"

rm "presets/$DST_ZIP"
rm -rf temp_folder
echo Unpacking "$SRC_ZIP"
unzip "$SRC_ZIP" -d ./temp_folder
cd temp_folder 
cp -r ../Factory_Presets_Extra/* ./
echo Repacking "../presets/$DST_ZIP"
zip -r9 "../presets/$DST_ZIP" .
cd ..
rm -rf temp_folder
