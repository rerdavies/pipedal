---
page_icon: img/Install4.jpg
---

{% include pageIcon.html %}


## Installing PiPedal


### Install for Ubuntu <br/>or Raspberry Pi OS (64-bit)


Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v1.4.70 Beta](https://github.com/rerdavies/pipedal/releases/download/)
- [Ubuntu 24.x (aarch64) v1.4.70 Alpha](https://github.com/rerdavies/pipedal/releases/download/)
- [Ubuntu 24.x (amd64) v1.4.70 Alpha](https://github.com/rerdavies/pipedal/releases/download/)


Version 1.4.70 has been tested on Raspberry Pi OS bookworm, Ubuntu 24.04 (amd64), and Ubuntu 24.10 (aarch64). Download the appropriate package for your platform, and install using the following proceduer:

```
  sudo apt update
  cd ~/Downloads  
  sudo apt-get install pipedal_1.4.70_arm64.deb 
```
Adjust accordingly depending which package you downloaded.

On Raspberry Pi OS, if you have a graphical desktop installed, you can also install the package by double-clicking on the downloaded package in the File Manager.


After installing, follow the instructions in [Configuring PiPedal after Installation](Configuring.md). If 
you are using Ubuntu, you will need to reconfigure Ubuntu to use a real-time kernel. 


--------
[<< System Requirements](SystemRequirements.md) | [Up](Documentation.md) | [Configuring PiPedal after Installation >>](Configuring.md)
