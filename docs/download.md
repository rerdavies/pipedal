## Download

# Download PiPedal 2.0.104-alpha


Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v2.0.104 Alpha](https://github.com/rerdavies/pipedal/releases/download/v2.0.104/pipedal_2.0.104_arm64.deb)
- [Ubuntu 24.x, 25.04 (aarch64) v2.0.104 Alpha](https://github.com/rerdavies/pipedal/releases/download/v2.0.104/pipedal_2.0.104_arm64.deb)
- [Ubuntu 24.x, 25.04 (amd64/x86_64) v2.0.104 Alpha](https://github.com/rerdavies/pipedal/releases/download/v2.0.104/pipedal_2.0.104_amd64.deb)


Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo apt install ./pipedal_2.0.104_arm64.deb  
        # or ... _amd64.deb as appropriate for your platform
```
The message about missing permissions given by `apt` is
expected, and can be safely ignored. Apt is unable to unpack the debian package in a sandbox because the 
package is a local file, rather than a file from the Internet. This is a known issue with apt, and does not indicate any problem with the package or installation process.

Follow the instructions in [_Configuring PiPedal After Installation_](https://rerdavies.github.io/pipedal/Configuring.html) to complete the installation.

# Download PiPedal 1.5.99 Stable Release

Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v1.5.99](https://github.com/rerdavies/pipedal/releases/download/v1.5.99/pipedal_1.5.99_arm64.deb)
- [Ubuntu 24.x, 25.04 (aarch64) v1.5.99](https://github.com/rerdavies/pipedal/releases/download/v1.5.99/pipedal_1.5.99_arm64.deb)
- [Ubuntu 24.x, 25.04 (amd64/x86_64) v1.5.99](https://github.com/rerdavies/pipedal/releases/download/v1.5.99/pipedal_1.5.99_amd64.deb)


Install the package by running 

```
  sudo apt update
  cd ~/Downloads  
  sudo apt install ./pipedal_1.5.99_arm64.deb  
        # or ... _amd64.deb as appropriate for your platform
```
The message about missing permissions given by `apt` is
expected, and can be safely ignored. Apt is unable to unpack the debian package in a sandbox because the 
package is a local file, rather than a file from the Internet. This is a known issue with apt, and does not indicate any problem with the package or installation process.


Follow the instructions in [_Configuring PiPedal After Installation_](https://rerdavies.github.io/pipedal/Configuring.html) to complete the installation.

