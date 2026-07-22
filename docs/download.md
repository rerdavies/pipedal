## Download

PiPedal 2.0.109 has been temporarily retracted. please use version 2.0 108.

# Download PiPedal 2.0.108


[Release Notes](https://rerdavies.github.io/pipedal/ReleaseNotes.html)

Download the most recent Debian (.deb) package for your platform:
 k
- [Raspberry Pi OS bookworm (aarch64) v2.0.108 Release](https://github.com/rerdavies/pipedal/releases/download/v2.0.108/pipedal_2.0.108_arm64.deb)
- [Ubuntu 24.x, 25.04 (aarch64) v2.0.108 Release](https://github.com/rerdavies/pipedal/releases/download/v2.0.108/pipedal_2.0.108_arm64.deb)
- [Ubuntu 24.x, 25.04 (amd64/x86_64) v2.0.108 Release](https://github.com/rerdavies/pipedal/releases/download/v2.0.108/pipedal_2.0.108_amd64.deb)


Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo apt install ./pipedal_2.0.108_arm64.deb  
        # or ... _amd64.deb as appropriate for your platform
```
The message about missing permissions given by `apt` is
expected, and can be safely ignored. Apt is unable to unpack the debian package in a sandbox because the 
package is a local file, rather than a file from the Internet. This is a known issue with apt, and does not indicate any problem with the package or installation process.

Follow the instructions in [_Configuring PiPedal After Installation_](https://rerdavies.github.io/pipedal/Configuring.html) to complete the installation.

