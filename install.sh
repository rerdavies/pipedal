#!/usr/bin/sudo bash
# copy files to installation directories
cmake --install build --prefix /usr --config Release -v
# Done as an install action by the debian package.
sudo /usr/bin/pipedalconfig --install  
# copy pipedalPluginProfile as well.
sudo cp build/src/pipedalProfilePlugin /usr/bin/pipedalProfilePlugin
chmod +X /usr/bin/pipedalProfilePlugin
