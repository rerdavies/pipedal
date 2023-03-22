---
page_icon: img/Compiling.jpg
---
## Building PiPedal From Source

{% include pageIcon.html %}

PiPedal has only been tested on Raspberry Pi OS, and Ubuntu, but should run with little or no modification on most Linux distributions. Pull requests to correct problems with building PiPedal on other versions of Linux are welcome. 

To build and debug PiPedal using Visual Studio Code, you need 8GB of memory. You can build PiPedal with 2GB of memory.

You should also be able to cross-compile PiPedal easily enough, but we do not currently provide support on how to do this. Visual Studio Code provides excellent support for cross-compiling, and good support for remote and cross-platform debugging, all of which should work with the PiPedal CMake build. However, management of ARM pakages on a Windows machine when cross-compiling is not great. Remote debugging may be a better path.

--------
[<< Which LV2 Plugins Does PiPedal Support?](WhichLv2PluginsAreSupported.md) | [Up](Documentation.md) | [Build Prerequisites >>](BuildPrerequisites.md)
