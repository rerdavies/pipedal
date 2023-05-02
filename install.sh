#!/usr/bin/bash
# copy files.
cmake --install build --prefix /usr --config Release -v
# Done as an install action by the debian package.
sudo /usr/bin/pipedalconfig --install  
