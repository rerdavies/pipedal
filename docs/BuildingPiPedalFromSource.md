## Building PiPedal From Source

PiPedal has only been tested on Raspbian, and Ubuntu, but should run with little or no modification on most Linux distributions. Pull requests to correct problems with building PiPedal on other versions of Linux are welcome. 

To build PiPedal, a Raspberry Pi 4B, with at least 4GB of memory is required (8GB recommended). You can build PiPedal from the command-line using CMake; but the project was originally build using
Microsoft Visual Studio Code. If you use VSCode, you will almost definitely need to hav 8GB of RAM.

You should also be able to cross-compile PiPedal easily enough, but we do not currently provide support on how to do this. Visual Studio Code provides excellent support for cross-compiling, and good support for remote
and cross-platform debugging, all of which should work with the PiPedal CMake build. In early development, PiPedal was built and debugged remotely on a Raspberry Pi, from a Windows desktop system. This should still 
more-or-less work, although you may have to scramble a bit to install some of the build dependencies.

--------
[<< Which LV2 Plugins Does PiPedal Support?](WhichLv2PluginsAreSupported.md) | [Build Prerequisites >>](BuildPrerequisites.md)