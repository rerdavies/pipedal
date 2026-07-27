#!/bin/bash
npm run build && \
#remove any existing .gz files.
if ls dist/assets/index*.js.gz 1> /dev/null 2>&1; then
    rm dist/assets/index*.js.gz
fi
# generate a .gz file for each index*.js file

for file in dist/assets/*.js; do
# generate a .gz file
    gzip -c $file > $file.gz
done
