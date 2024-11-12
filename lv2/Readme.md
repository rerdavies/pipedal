This folder contains pre-built binaries for an initial bundle of Lv2 plugins.

This arrangment is temporary, and is expected to go away, once a reliable distribution 
of this project and the ToobAmp project via apt can be arranged. 

To update the arm64 binaries for ToobAmp, build the ToobAmp project, run `./install.sh` in the ToobAmp project directory,
and then run `./provisionLv2` in the pipedal root directory. This copies the ToobAmp binaries into the pipedal source tree.

The x86_64 binaries are, instead built from an artifact provided by the github CMake build action. Currently, these 
binaries are built on an Ubuntu 24.04 LTS build runner. PiPedals's build runner should run on the same build runner 
configuration. The CMake build process will unpack the artifact, and extract ToobAmp LV2 binaries into the build directory
where the debian package can pick them up.


