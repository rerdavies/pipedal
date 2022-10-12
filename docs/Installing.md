---
page_icon: img/Install4.jpg
---

{% include pageIcon.html %}


## Installing PiPedal


### Install for Ubuntu <br/>or Raspberry Pi OS (64-bit)


Download the most recent Debian (.deb) package for your platform:

- [Ubuntu/Raspberry Pi OS (64-bit) v1.0.17](https://github.com/rerdavies/pipedal/releases/download/v1.0.17/pipedal_1.0.17_arm64.deb)

Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo dpkg --install pipedal_1.0.17_arm64.deb
```
On Raspberry Pi OS, if you have a graphical desktop installed, you can also install the package by double-clicking on the downloaded package in the File Manager.


After installing, follow the instructions in [Configuring PiPedal after Installation](Configuring.md).


--------
[<< System Requirements](SystemRequirements.md) | [Up](Documentation.md) | [Configuring PiPedal after Installation >>](Configuring.md)
