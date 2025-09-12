## Download

### Install for Ubuntu or Raspberry Pi OS (64-bit)

Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v1.5.92 Beta](https://github.com/rerdavies/pipedal/releases/download/v1.5.92/pipedal_1.5.92_arm64.deb)
- [Ubuntu 24.x, 25.04 (aarch64) v1.5.92 Alpha](https://github.com/rerdavies/pipedal/releases/download/v1.5.92/pipedal_1.5.92_arm64.deb)
- [Ubuntu 24.x, 25.04 (amd64) v1.5.92 Alpha](https://github.com/rerdavies/pipedal/releases/download/v1.5.92/pipedal_1.5.92_amd64.deb)


Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo apt-get install ./pipedal_1.5.92_arm64.deb
```
You MUST use `apt-get` to install the package. `apt install` will NOT install the package correctly. The message about missing permissions given by `apt-get` is
expected, and can be safely ignored.

Follow the instructions in [_Configuring PiPedal After Installation_](https://rerdavies.github.io/pipedal/Configuring.html) to complete the installation.
