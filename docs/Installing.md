---
page_icon: img/Install4.jpg
---

{% include pageIcon.html %}


## Installing PiPedal


### Install for Ubuntu <br/>or Raspberry Pi OS (64-bit)


Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (64-bit) v1.3.56](https://github.com/rerdavies/pipedal/releases/download/)
- [Ubuntu/Raspberry Pi OS bullseyeye (64-bit) v1.2.31](https://github.com/rerdavies/pipedal/releases/download/v1.1.31/pipedal_1.1.31_arm64.deb)

Version 1.3.56 has not yet been tested on Ubuntu or Raspberry Pi OS bullseye. On these platforms, we recommend that you use version 1.1.31.

Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo apt-get install pipedal_1.3.56_arm64.deb 
```
Adjust accordingly if you have downloaded v1.1.31.

On Raspberry Pi OS, if you have a graphical desktop installed, you can also install the package by double-clicking on the downloaded package in the File Manager.


After installing, follow the instructions in [Configuring PiPedal after Installation](Configuring.md).


--------
[<< System Requirements](SystemRequirements.md) | [Up](Documentation.md) | [Configuring PiPedal after Installation >>](Configuring.md)
