# PiPedal v0.0,1 (Alpha)

![Screenshot](artifacts/PiPedalSshots.png)

PiPedal is a multi-effect guitar pedal for Raspberry Pi devices, running Raspbian, or Ubuntu. It is controlled via a web interface that has been designed to work well with small-form-factor devices like phones and tablets.

You will need a suitable audio input/output device to use PiPedal, which can be either an external USB audio device, or a Raspberry Pi ADC/DAC hat, providing at least one input and one output audio channel.

PiPedal is controlled via a web app that's been designed to support small-form-factor devices, like phones and tablets, although it works gloriously with desktop browers as well. You should not have to carry around a laptop to control your PiPedal when you're out gigging; and the web interface for PiPedal has been designed with that scenario specifically in mind. Just connect your phone to the PiPedal Wi-Fi hotspot and you have complete control over your PiPedal.

PiPedal uses LV2 audio plugin effects. You will need to install LV2 plugins before you can get started. See the LV2 Plugins section, below, for a list of good plugin collections to get started with.

You can add as many plugins to your patch as your CPU will support (well over a dozen on a Raspberry Pi 4+). Signal chains
can have an arbitrary number of split chains, which may be A/B-selected or mixed if you wish.

Bind MIDI controls and notes to LV2 plugin controls using the Midi Bindings dialog. USB micro controllers such as the Korg nano series of MIDI controllers, or the AKAI pad controllers are perfectly suited for selecting and tweaking patches while performing; or you can connect regular MIDI controllers and pedalboards via the MIDI ports on your USB Audio device, if it has them.

## System Requirements

* A Raspberry PI 4B or 400, with at least 2GB of RAM to run, and at least 4GB of RAM to build.
* An external USB Audio Adapter, or a Pi audio hat with at least one audio input, and one audio output.

PiPedal has been tested on the following Operating Systems:

* Raspbian 32-bit
* Ubuntu Gnome3 21.04 32-bit or 64-bit (Ubuntu KDE will not work)

But it should work on most Debian-derived Linux variants.

If you are using Rasbian, make sure to upgrade to the latest version, because versions of the Linux kernel later than 5.10 provide dramatically improved support for external USB audio devices.

An RT_PREEMPT kernel does provide better latency; but modern PREEMPT kernels provide enough real-time support to run PiPedal without problems.

## Latency

Note that Pipedal is not intended for use when logged in to Raspbian. Screen updates and heavy filesystem activity will cause audio dropouts. For best results, access PiPedal using the web interface remotely, through the Wi-Fi hotspot. Accessing the web interface via Wi-Fi has little or no effect on audio latency or dropouts.

With a good USB audio device, PiPedal should be able to provide stable audio with 4ms (good), or 2ms (excellent) latency on a Raspberry Pi 4 when running on a Realtime kernel. Your actual results may vary.

The current Linux kernel provides best latency on USB audio devices when they are configured with a sampling rate of 48kHZ, and 3 buffers. Cheap USB audio devices (e.g. M-Audio  M-Track Solo, available for less than $60) should be able to run without dropouts at 48kHz with 3x64 sample buffers. Most devices in this class use the same Burr-Brown chipset. Premium USB Audio devices should run stably at 48kHZ 3x32 sample buffers (about 4ms latency). I personally use the MOTU M2 USB audio adapter, which I highly recommmend -- stable, quiet, low-latency, great controls, and built like a tank).

Make sure your system is fully updated, and that you are running with a kernel version of 5.10 or later, since version 5.10 of the Linux kernel incorporates significantly improved support for class-compliant USB audio devices. The MOTU M2 will not run on versions of the kernel prior to 5.10.

RT_PREEMPT realtime kernels are recommended but not required. PiPedal provides better (but not dramatically better) latency when running on a Raspbian Realtime kernel. Stock Raspbian provides PREEMPT real-time scheduling, but does not currently have all of the realtime patches, so interrupt latency is slightly more variable on stock Rasbian than on custom builds of Raspbian with RT_PREEMPT patches applied. If you want to install a realtime kernel on Raspian, please visit

    https://github.com/kdoren/linux/releases/tag/5.10.35-rt39
    
As of September 2021, this site provides a working Realtime kernel for Raspbian 5.10 on Raspberry Pi 3, 3+ and 4, with support for other versions in progress. This kernel may allow you to run with 4ms latency instead of 8ms.

The Ubuntu Studio installer will install a realtime kernel if there is one avialable. But -- at least for Ubuntu 21.04 -- there is no stock RT_PREEMPT kernel for ARM aarch64.

On a Raspberry Pi 4 device, Wi-Fi, USB 2.0, USB 3.0 and SDCARD access are performed over separate buses (which is not true for previous versions of Raspberry Pi). It's therefore a good idea to ensure that your USB audio device is either the only device connected to the USB 2.0 ports, or the only device connected to the UBS 3.0 ports. There's no significant advantage to using USB 3.0 over USB 2.0 for USB audio. TCP/IP network traffic does not seem to adversely affect USB audio operation. Filesystem activity does affect USB audio operation on Rasbian, even with an RT_PREEMPT kernel; but interestingly, filesystem activity has much less effect on UBS audio on Ubuntu 21.04, even on a plain PREEMPT kernel. 

There is some reason to beleive that there are outstanding issues with the Broadcom 2711 PCI Express bus drivers on Rasbian realtime kernels, but as of September 2021, this is still a research issue. If you are brave, there is strong annecdotal evidence that these issues arise when the Pi 4 PCI-express bus goes into and out of power-saving mode, which can be prevented by building a realtime kernel with all power-saving options disabled. But this is currently unconfirmed speculation. And building realtime kernels is well outside the scope of this document. (source: a youtube video on horrendously difficult bugs encountered while supporting RT_PREEMPT, by one of the RT_PREEMPT team members).

For the meantime, for best results, log off from your Raspberry Pi, and use the web interface only.

## Configuring PiPedal After Installation

After PiPedal is installed, you can connect to the web interface as follows: via the mDNS address "http://pipedal.local" (on Windows, Mac, or iPhone web 
browsers), at http://127.0.0.1 if you are interactively logged into your Raspberry Pi device, or at port 80 of the current network address of your 
Raspberry Pi, if you are connected from an Android device (which does not currently support mDNS). 

Android devices do not support mDNS. If you are connecting to PiPedal with an Android device, via the Host Access Point, PiPedal, the PiPedal user interace can be reached at http://172.22.1.1 If you are connecting via the Raspberry Pi's Ethernet port, connect to http://*address of your Pi*:80

To complete the initial configuration, you must either connect an Ethernet cable to your Raspberry pi so you can connect to the Web App (after which you should be able to connect to http://pipedal.local); or you must launch a web browser on your Raspberry pi device while logged in interactively. 
     
If you already have another web server on port 80, see the section "Changing the Web Server Port", below.

Once connected, select the Settings menu item on the Hamburger menu at the top left corner of the display. Click on Audio Device Settings to select and configure the audio device you want to use. 

You can also activate PiPedal's Wi-Fi hotspot connection from the Settings dialog. Click on the Wifi Hotspot menu item in the Settings dialog.

     IMPORTANT NOTE: Activating the WiFi hotspot will DISABLE outbound Wi-Fi connections from the Raspberry Pi.
     You will be able to access the PiPedal web interface through the hotspot connection, and make ssh and VNC 
     connections to the Raspberry Pi through the hotspot connection; but your Raspberry Pi will not have 
     outbound access to the Internet, unless an Ethernet cable is connected to the Raspberry Pi.

If you need access to the internet once the hotspot has been enabled, connect an Ethernet cable to
the Raspberry Pi. Note that the PiPedal hotspot is NOT configured to forward internet traffic from the Wi-Fi hotspot to the LAN connection, and 
generally, the Raspberry Pi will not be able to access the internet via devices connected to the Wi-Fi hotspot. Consult documentation for hostapd 
if you want to do this.

There are a number of other useful settings in the hamburger menu/Settings dialog. For example, most USB audio devices route instrument
input onto the right channel of the USB audio inputs. So you probably want to configure PiPedal to use only the right USB audio input channel. 
You can choose how to bind USB audio input and output channels (stereo, left only, right only) in the settings dialog. If you are using a Audio 
device that has more than two channels, you will be offered a list of channels to choose from instead.

## Running on Ubuntu

When running on stock Ubuntu, you should install Ubuntu Studio addons and enable the low-latency settings and performance tweaks options.

    sudo apt install ubuntustudio-installer
    
You probably want to install the Audio Plugins options as well. 

To get PiPedal to work properly on Ubuntu while not logged on, you must remove PulseAudio

    sudo apt remove pulseaudio
    
If you choose not to do that, it is possible to use PiPedal with pulseaudio installed, but you will have to start and stop the jack audio service installed by PiPedal manually 
    
    sudo pipedalconfig --restart

which will kill the Pulse Audio daemon as part of the restart process.

## Command Line Configuration of PiPedal

The pipedalconfig program can be used to modify configuration of PiPedal from a shell command line. Run 'pipedalconfig --help' to view
available configuration commands, some of which are not avaialbe from the web interface. For example, you can change the port number
of the Web App HTTP server if you need to, using pipedalconfig.
     
Things you can do with pipedalconfig:
     
     - Stop, start or restart the PiPedal services.
     
     - Choose whether to automatically start PiPedal services on reboot.
     
     - Select an alternate web server port.
     
     - enable or disable the Wi-Fi hotspot.
     
Run `pipedalconfig --help` for available options.
     

## Changing the web server port.
     
If your Pi already has a web server on port 80, you can reconfigure PiPedal to use a different port. After installing PiPedal, run the following command on your Raspberry Pi:
     
     sudo pipedalconfig --install --port 81
     
You can optionally restrict the addresses on which PiPedal will respond by providing an explicit IP address. For example, to configure PiPedal to only accept connections from the local host:
     
     sudo pipedalconfig --install --port 127.0.0.1:80
     
To configure PiPedal to only accept connections on the Wi-Fi host access point:
     
     sudo pipedalconfig --install --port 172.22.1.1:80


## LV2 PLugins

PiPedal uses standard LV2 audio plugins for effects. There's a huge variety of LV2 guitar effect plugins, and plugins collections, many 
of which are specifically intended for use as guitar effect plugins. Ubuntu Studio comes with an enormous collection of LV2 plugins preinstalled. On Rasbian, you will have to manually select and install the plugins you want to use.

On stock Ubuntu, you can install Ubuntu Studio addons (sudo apt install ubuntu-studio-install), and ask it to install Audio Plugins. This will install a large collection of LV2 plugins, which will be automatically detected by PiPedal. Or you can choose your LV2 plugin collections manually, as for Rasbian.

The following LV2 Plugin collections (all recommended) are available on both Rasbian and Ubuntu.

  sudo apt install guitarix-lv2     # https://guitarix.org/
  
  sudo apt install zam-plugins    # http://www.zamaudio.com/
  
  sudo apt install mda-lv2  # http://drobilla.net/software/mda-lv2/
  
  sudo apt install calf-plugins  # http://calf-studio-gear.org/
  
  sudo apt install fomp  # http://drobilla.net/software/fomp/

But there are many more.

Visit https://lv2plug.in/pages/projects.html for more suggestions.

There is also a set of supplementary Gx effect plugins which did not make it into the main Guitarix distribution. You will
have to compile these plugins yourself, as they are not currently avaiable via apt. But if you are comfortable building
packages on Raspbian, the effort is well worthwhile. The GxPlugins collection provides a number of excellent boutique amp emulations,
as well as emulations of famous distortion effect pedals that are not part of the main Guitarix distribution.

- https://github.com/brummer10/GxPlugins.lv2

## Which LV2 Plugins does PiPedal support?

PiPedal will automatically detect installed LV2 plugins and make them selectable from the web app interface, as long as they meet the 
following conditions:

- Must have mono or stereo audio inputs and outputs.

- Must not be MIDI instruments or have CV (Control Voltage) inputs or outputs.

- Must be remotely controllable (no hard dependency on GUI-only controls), which is true of the vast majority of LV2 plugins.

If you install new LV2 plugins, you will have to restart the PiPedal web service (or reboot the machine) to get them to show up in the web interface.

   sudo pipedalconfig --restart

Although most LV2 plugins provide GUI interfaces, when running on a LINUX desktop, the LV2 plugin standard is specifically designed to allow remote control 
without using the provided desktop GUI interface. And all but a tiny minority of LV2 plugins (most of them analyzers, unfortunately) support this.

## Building and Installing PiPedal

PiPedal has only been tested on Raspbian, and Ubuntu. But pull requests to correct problems with building PiPedal on other versions of Linux are welcome. 

To build PiPedal, a Raspberry Pi 4B, with at least 4GB of memory is recommended. You should be able to cross-compile PiPedal easily enough,
but we do not currently provide support on how to do this. Visual Studio Code provides excellent support for cross-compiling, and good support for remote
debugging, which will work with the PiPedal build. Or consult CMake documentation on how to cross-compile source.

### Build Prerequisites

Run the following commands to install build tools required by the PiPedal build.

    # install CMake
    sudo apt update
    sudo apt install cmake ninja-build

The PiPedal build process also requires version 14 or later of `node.js`. Type `node --version` to see if you have a version 
of `node.js` installed already. Otherwise run the following commands as root to install the v14.x lts version of `node.js`: 

    # install NodeJS lastest LTS release.
    curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
    sudo apt-get install -y nodejs


Run the following commands to install dependent libraries required by the PiPedal build.

    sudo apt update
    sudo apt install liblilv-dev libboost-dev libjack-jackd2-dev libnl-3-dev libnl-genl-3-dev libsystemd-dev catch
    sudo apt install libasound2-dev
    sudo apt install libwebsocketpp-dev

Run the following command to install and configure React dependencies.

   ./react-config   # Configure React dependencies.
  
### Building PiPedal from Source

PiPedal was developed using Visual Studio Code. Using Visual Studio Code to build PiPedal is recommended, but not required. The PiPedal
build procedure uses CMake project files, which are supported by most major Integrated Development Environments.
     
If you open the PiPedal project as a folder in VS Code, Code will 
detect the CMake configuration files, and automatically configure the project to use available toolchains. Once the VS Code CMake plugin (written by Microsoft,
available through the plugins store) has configured itself, build commands and configuration options should appear on the bottom line of Visual Studio 
Code. 

If you are not using Visual Studio Code, you can configure, build and install PiPedal using CMake build tools. For your convenience,
the following shell scripts have been provided in the root of the project in order to provide convenent CLI build commands.

    sudo ./init    # Configure the CMake build (required if you change one of the CMakeList.txt files)

    ./mk   # Build all targets.
    
    sudo ./install   # Deploy all targets 
    
    ./pkg    # Build a .deb file for distribution.
    
If you are using a development environment other than Visual Studio Code, it should be fairly straightforward to figure out how
to incorporate the PiPedal build procedure into your IDE workflow by using the contents of the build shell scripts as a model.

The CMake toolchain can be used to generate build scripts for other build environments as well (e.g. Visual Studio .csproj files, Ant, or Linux
 Makefiles); and can be configured to perform cross-compiled builds as well. Consult documentation for CMake for instructions on how to
do that if you're interested. Visual Studio Code also provides quite painless procedures for cross-compiling CMake projects, and mostly-painless 
procedures for remote building, and/or debugging. If you need to build for platforms other than Raspbian, or build on platforms other than Rasbian, 
you may want to investigate what Visual Studio Code's CMake integration provides in that regard.

### How to Debug PiPedal.

PipPedal consists of the following subprojects:

*    A web application build in React, found in the react subdirectory.

*    `pipedald`: a Web server, written in C++, serving a web socket, and pre-built HTML components from the React app.
     All audio services are provided by the pipedal process.

*   `pipedalshutdownd`: A service to execute operations that require root credentials on behalf of pipedald. (e.g. shutdown, reboot,
    and pushing configuration changes).

*   `pipedalconfig`: A CLI utility for managing and configuring the pipedald services.
     
*   `pipedaltest`: Test cases for pipedald, built using the Catch2 framework.


You must stop the pipdeal service before launching a debug instance of pipedald:

    sudo systemctl stop pipedald

or

    pipedalconfig -stop  #Stops the Jack service as well.

But there's no harm in running a debug react server that's configured to connect to the web 
socket of a production instance of pipedal on port 80, if you aren't planning to debug C++ code.

In production, the pipedald web server serves the PiPedal web socket, as well as static HTML from the  built 
react components. But while debugging, it is much more convenient to use the React debug server for 
React sources, and configure pipedald to serve only the websocket. 

To start the React debug server, from a shell, cd to the react directory, and run "./start". The react debug 
server will detect any changes to React sources, and rebuild them automatically (no build step required). 
Actual debugging is performed using the Chrome debugger (which is remarkably well integrated with React).

To get this to work on Raspberry Pi, you will probably have to make a configuration change.

Edit the file `/etc/sysctl.conf`, and add or increase the value for the maximum number of watchable user 
files:

    fs.inotify.max_user_watches=524288

followed by `sudo sysctl -p`. Note that VS Code and the React framework both need this change.

By default, the React app will attempt to contact the pipedal server on ws:*:8080 -- the address on which
the debug version of systemctld listens on. This can be reconfigured
in the file react/src/public/var/config.json if desired. If you connect to the the pipedald server port, pipedald intercepts requests for this file and 
points the react app at itself, so the file has no effect when running in production. 

The React app will display the message "Error: Failed to connect to the server", until you start the pipedal websocket server in the VSCode debugger. It's quite reasonable to point the react debug app at a production instance of the pipedal server instead.

    react/public/var/config.json: 
    {
        ...
        "socket_server_port": 80,
        "socket_server_address": "*",
        ...
    }

Setting socket_server_address to "*" configures the web app to reconnect using the host address the browser
request used to connect to the web app. (e.g. 127.0.0.1, or pipedal.local, &c). If you choose to provide an explicit address,
remember that it is to that address that the web browser will connect.

The original development for this app was done with Visual Studio Code. Open the root project directory in
Visual Studio Code, and it will detect the CMake build files, and configure itself appropriately. Wait for 
the CMake plugin in Visual Studio Code to configure itself, after loading. 

Once CMake has configured itself, build and debug commands are available on the CMake toolbar at the 
bottom of the Visual Studio Code window. Set the build variant to debug. Set the debug target to "pipedald". 
Click on the Build button to build the app. Click on the Debug button to launch a debugger.

To get the debugger to launch and run correctly, you will need to set commandline parameters for pipedald. 
Commandline arguments can be set in the file .vscode/settings.json: 

    {
        ...
        "cmake.debugConfig": {
            "args": [
            "<projectdirectory>/debugConfig","<projectdirectory>/build/react/src/build",  "-port", "0.0.0.0:8080"
            ],

        ...
    }

where `<projectdirectory>` is the root directory of the pipedal project.

The default debug configuration for pipedal is configured to use /var/pipedal for storing working data files, 
which allows it to share configuration with a production instance of pipedal. Be warned that the permissioning 
for this folder is intricate. If you plan to use the data from a production server, get the production server 
installed first so the permissions are set correctly. If you install a production instance later, remove the 
entire directory before doing so, to ensure that none of the files in that directory are permissioned 
incorrectly. 

You will need to add your userid to the pipedal_d group if you plan to share the /var/pipedal directory. 
     
     sudo usermod -a -G pipedal_d *youruserid*

Or you can avoid all of this, by configuring the debug instance to use a data folder in your home directory. Edit 
`debugConfig/config.json`:

    {
        ...
        "local_storage_path": "~/var/pipedal",
        ...
    }

