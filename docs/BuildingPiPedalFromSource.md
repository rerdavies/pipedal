---
page_icon: img/Compiling.jpg
---
## Building PiPedal From Source

{% include pageIcon.html %}

PiPedal has only been tested on Raspberry Pi OS, and Ubuntu 24.x, but should run with some modifications on most Linux distributions. Pull requests to correct problems with building PiPedal on other versions of Linux are welcome. However, getting build pre-requisites installed on non-Debian-derived Linux distributions (distros that don't use `apt`) will be challenging.

To build and debug PiPedal using Visual Studio Code, you need 8GB of memory. You can build PiPedal with 2GB of memory.

If your Pi is short on memory, you _can_ run VSCode on laptop or desktop and configure VSCode to connect to your Raspberry PI using SSH connections. You can definitely build PiPedal using remote SSH connections on a 6GB Ubuntu VM; and can probably build Pipedal using SSH connections on a 4GB Raspberry Pi, as long as you are careful.

As a last resort, you be able to configure VSCode and/or CMake to cross-compile PiPedal on a laptop or desktop computer; but installing build dependencies when cross-compiling will be extremely challenging.  This is not a scenario that we can provide support for.



--------
[<< Which LV2 Plugins Does PiPedal Support?](WhichLv2PluginsAreSupported.md) | [Up](Documentation.md) | [Build Prerequisites >>](BuildPrerequisites.md)
