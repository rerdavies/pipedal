---
page_icon: img/Requirements2.jpg
---
## System Requirements

{% include pageIcon.html %}

### Processor and Memory Requirements

* An ARM64 or x86-64 CPU. Any of the following provide excellent performance.    
   * Raspberry Pi 5, Pi 500, Pi 500+ (or equivalent ARM64 single-board computer). Recommended.
   * Intel N100-class NUCs or mini computers. Also recommended.
   * Most AMD/Intel laptop or desktop CPUs. 
* PiPedal will also run well on a Raspberry Pi 4.
*  A Raspberry Pi 3 (or equivalent SBC) can run PiPedal, but may struggle to keep up. Not recommended. 
* At least 2GB of RAM (4GB recommended). 
* To compile PiPedal from source, at least 8GB of RAM is required.

### Audio System Requirements:
* On Raspberry Pi's, you will need an external USB Audio Adapter, or a Pi audio hat with at least one audio input. You cannot use the built-in audio inputs and outputs. 
* On x86-64 systems, any standard audio interface with at least one input and one output will work; but an external USB audio interfaces is highly recommended. 

### Operating System Support

PiPedal has been tested on the following Operating Systems:

* Raspberry Pi OS 64-bit Bookworm, or Trixie. 
* Ubuntu 24.x or later, 64-bit (amd64/x86-64 and aarch64). Make sure you follow the [Ubuntu post-install instructions](https://rerdavies.github.io/pipedal/Configuring.html) to ensure your Ubuntu OS is using a realtime kernel. Pipedal will also run on Ubuntu Server 24.x or later. 


PiPedal may also run on other Debian 12 or Debian 13-based Linux distributions, but these have not been tested, and we can provide only limited support for these distributions, since we are not able to locally test every possible Linux distribution. 

You may be able to compile Pipedal from sources on non-Debian Linux distributions, but working out compiler and library dependencies on non-Debian-based Linux distributions may be difficult, and is not officially supported. If you do manage to compile PiPedal on a non-Debian-based Linux distribution, please let us know how you did it, and we will publish instructions for others that follow in your footsteps.
 
A Linux kernel version of 5.15 or later is required, since support for USB audio adapters is poor in prior kernel versions. A Linux kernel version of 6.15 or later is recommended. 

PiPedal requires a realtime-capable Linux kernel (a PREEMPT_RT kernel). Current versions of Ubuntu can be configured to enable PREEMPT_RT at runtime. (See the [Ubuntu post-install instructions](https://rerdavies.github.io/pipedal/Configuring.html) for details). Raspberry Pi OS 64-bit Bookworm and Trixie include a PREEMPT_RT kernel by default, so no additional configuration is required to enable PREEMPT_RT on these OS versions.

### Headless Operation

PiPedal provides best performance and audio latency when controlled from a remote browser, since GPU activity can interfere with real-time audio processing. The PiPedal server does not require a graphical desktop, so headless installs are fully supported (and more-or-less recommended). You can completely install and configure PiPedal using Linux commandline alone; however, you may find it easier to set up, configure and maintain your PiPedal server if you have a desktop environment installed. 

You should not run Pipedal using a browser on the same machine that is running the PiPedal server, unless you have a relatively performant desktop or laptop class machine. 

The Pipedal server does update itself automatically (it prompts for permission from the browser interface if there is an update available). However, it does not take care of Operating System updates. You should make sure to keep your OS up to date, and to install security updates as they become available. You will need to do that yourself from a commandline terminal, or using a desktop environment on the machine running the PiPedal server.

You may find it much easier to set up and maintain your PiPedal server if you enable SSH access on the server machine.

### Older Versions of PiPedal

Older versions of PiPedal (v1.1.31) have been tested on the following operating systems, but are no longer supported (and are not really recommended):

* Ubuntu 21.04 or later, 64-bit
* Raspberry PI OS 64-bit bullseye


--------
[<< Changing the Web Server Port](AboutPiPedal.md)  | [Up](Documentation.md) | [Installing PiPedal >>](Installing.md)

