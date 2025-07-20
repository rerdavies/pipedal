---
page_icon: img/Install4.jpg
---

{% include pageIcon.html %}


## Installing PiPedal


### Install for Ubuntu <br/>or Raspberry Pi OS (64-bit)


Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v1.4.78](https://github.com/rerdavies/pipedal/releases/download/v1.4.78/pipedal_1.4.78_arm64.deb)
- [Ubuntu 24.x (aarch64) v1.4.78](https://github.com/rerdavies/pipedal/releases/download/v1.4.78/pipedal_1.4.78_arm64.deb)
- [Ubuntu 24.x (amd64) v1.4.78](https://github.com/rerdavies/pipedal/releases/download/v1.4.78/pipedal_1.4.78_amd64.deb)


Version 1.4.78 has been tested on Raspberry Pi OS bookworm, Ubuntu 24.04 (amd64), and Ubuntu 24.10 (aarch64). Download the appropriate package for your platform, and install using the following procedure:

```
  sudo apt update
  sudo apt upgrade
  cd ~/Downloads  
  sudo apt-get install ./pipedal_1.4.78_arm64.deb 
```
You MUST use `apt-get`. `apt` will not install downloaded packages; and `dpkg -i` will not install dependencies. 

On Raspberry Pi OS, if you have a graphical desktop installed, you can also install the package by double-clicking on the downloaded package in the File Manager.

If you are using Ubuntu, there are additional post-installation steps you must take to reconfigure the Ubuntu kernel to 
provide support for low-latency real-time audio. Proceed to [PiPedal support for Ubuntu](Ubuntu.md).

If you are using Raspberry Pi OS, proceed directly to [Configuring PiPedal after Installation](Configuring.md). If 
you are using Ubuntu, there are additional post-installation steps you must take to reconfigure the Ubuntu kernel 
to provide support for low-latency real-time audio. 


--------
[<< System Requirements](SystemRequirements.md) | [Up](Documentation.md) | [PiPedal on Ubuntu >>](Ubuntu.md)
