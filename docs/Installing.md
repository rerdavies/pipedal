---
page_icon: img/Install4.jpg
---

{% include pageIcon.html %}


## Installing PiPedal


### Install for Ubuntu <br/>or Raspberry Pi OS (64-bit)


Download the most recent Debian (.deb) package for your platform:

- [Raspberry Pi OS bookworm (aarch64) v2.0.106](https://github.com/rerdavies/pipedal/releases/download/v2.0.106/pipedal_2.0.106_arm64.deb)
- [Ubuntu 24.04 through 25.04 (aarch64) v2.0.106](https://github.com/rerdavies/pipedal/releases/download/v2.0.106/pipedal_2.0.106_arm64.deb)
- [Ubuntu 24.04 through 25.04 (amd64) v2.0.106](https://github.com/rerdavies/pipedal/releases/download/v2.0.106/pipedal_2.0.106_amd64.deb)


Version 2.0.106 has been tested on Raspberry Pi OS bookworm, Ubuntu 24.04 (amd64), Ubuntu 24.10 (aarch64), and Ubuntu 25.04 (aarch64). Download the appropriate package for your platform, and install using the following procedure:

```
  sudo apt update
  sudo apt upgrade
  cd ~/Downloads  
  sudo apt-get install ./pipedal_2.0.106_arm64.deb 
```
You MUST use `apt-get`. `apt` will not install downloaded packages; and `dpkg -i` will not install dependencies. 

On Raspberry Pi OS, if you have a graphical desktop installed, you can also install the package by double-clicking on the downloaded package in the File Manager.

If you are using Ubuntu, there are additional post-installation steps you must take to reconfigure the Ubuntu kernel to 
provide support for low-latency real-time audio. Proceed to [PiPedal support for Ubuntu](Ubuntu.md).

If you are using Raspberry Pi OS, proceed directly to [Configuring PiPedal after Installation](Configuring.md). If 
you are using Ubuntu, there are additional post-installation steps you must take to reconfigure the Ubuntu kernel 
to provide support for low-latency real-time audio. 

### Installing PiPedal on a Server that Does not have a Graphical Desktop Environment

The easiest way to do this is to download the appropriate Debian package for your platform to your desktop computer, and then copy the downloaded package to the server using `scp` or a similar file transfer method. Windows 10 and 11 provide a pre-installed `scp` command that you can use from the Command Prompt or PowerShell. On Linux desktops you may need to install `scp` using:

```
sudo apt install openssh-client
```
You must first ensure that you have SSH access to the server. Raspberry Pi OS and Ubuntu both provide a way to ensure that SSH is installed and running at first boot. First, ensure that you have SSH access to the server. If you have not yet set up your Raspberry Pi, the Raspberry Pi Imager application, and the Raspberry Pi Network Install utility allow you to pre-install and enable SSH on first boot. 

See [the Raspberry Pi official documentation](https://www.raspberrypi.com/documentation/computers/getting-started.html#install) for detailed information on how to enable SSH on first boot. Navigate to the Customisation section of the Raspberry Pi Imager, and configure a default username and password. Then select the "Remote Access" subsection of the Customization section, and enable SSH. You can do this from either the Raspberry Pi Imager application, or when configuring your Raspberry Pi OS using the Network Boot installer. You can also enable SSH by creating an empty file named `ssh` in the root of the boot partition of the SD card after you have flashed the OS image to the SD card, as long as you have not yet booted using the SD card.

On Ubuntu, you will typically have had to attach a keyboard and monitor to complete the initial installation anyway. Consult Ubuntu documentation for how to enable SSH access to your Ubuntu server.


Once you have SSH access to your PiPedal server, you should download the appropriate Debian package for your platform to your desktop machine. You can then copy the downloaded package to the server using `scp` or a similar file transfer method. Note that Windows 10 and 11 provide a pre-installed `scp` command that you can use from the Command Prompt or PowerShell. On Linux desktops you may need to install `scp` using:

```
sudo apt install openssh-client
```

Use the following command to copy the downloaded package to the server from Linux or Windows. Windows 10 and 11 have `scp` installed by default. 

Adjust `username`, `server_address` and the actual name of the Debian package you downloaded as needed:

```
scp Downloads/pipedal_2.0.106_arm64.deb username@server_address:/home/username/
```
This will copy the downloaded package on your desktop computer to your home directory on the PiPedal server computer. 

Once you have copied the package to the server, you can SSH into the server and install the package using `apt-get`:

```
ssh username@server_address
sudo apt-get install ./pipedal_2.0.106_arm64.deb
```
The PiPedal package installer will print out the port number that the PiPedal web server is listening on. It defaults to port 80, but if you have another web server already running on port 80, it will select the next available port. Ubuntu default installs have Apache Server running on port 80, so the PiPedal web server will default to port 81 on Ubuntu.

### Acessing the PiPedal Web Application after Installation


The PiPedal server has now been installed, and should be up and running. You can access the PiPedal web application by navigating to `http://server_address:port_number` in a web browser on your desktop computer. 

If you are using Ubuntu, see the [next section of the documentation](Ubuntu.md) for steps that you must take to before using the PiPedal Web Application. 

Windows, Linux, and MacOS all support MDNS network naming. You can connect to your PiPedal server on Raspberry Pi OS by navigating to    

   http://raspberrypi.local 

(raspberrypi.local is the default hostname for a Raspberry Pi OS installation; adjust the hostname as appropriate, if you changed the hostname at pre-configuration time). 

On Ubuntu, you will have had to select a custom hostname during installation. You can connect to your PiPedal server on Ubuntu by navigating to 

   http://yourservername.local:81






--------
[<< System Requirements](SystemRequirements.md) | [Up](Documentation.md) | [PiPedal on Ubuntu >>](Ubuntu.md)
