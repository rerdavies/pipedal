# PiPedal

PiPedal is a multi-effect guitar pedal for Raspberry Pi devices. You will need a suitable audio input/output device to use PiPedal, which can be either an external USB audio adapter, or a Raspberry Pi ADC/DAC hat, providing at least one input and one output audio channel.

PiPedal is controlled via a clean compact web app that's suitable for use on small-form devices, like phones and tablets, although it works gloriously with desktop browers as well. You should not have to carry around a laptop to control your PiPedal when you're out gigging; and the web interface for PiPedal has been designed with that scenario specifically in mind. PiPedal has been designed with compact display formats, and touch user-interfaces in mind. Just connect to the PiPedal Wi-Fi access point with your phone, and you have complete control over your PiPedal.

PiPedal effects are LV2 audio plugins. There are currently a wide variety of sources for excellent LV2 guitar plugins. 

You can add as many plugins to your patch as your CPU will support (well over a dozen on a Raspberry Pi 4+). Signal chains
can have an arbitrary number of split chains, which maybe A/B-selected or mixed if you wish.

PiPedal allows you to easily bind Midi controls and notes to LV2 plugin controls using the Midi Bindings dialog. USB micro controllers such as the Korg nano series of MIDI controllers, or the AKAI pad controllers are perfectly suited for selecting and tweaking patches while performing; or you can connect regular MIDI controllers and pedalboards via the MIDI ports on your USB Audio device, if it supports MIDI. 

## Configuring PiPedal After Installation

After PiPedal is installed, you can connect to the web interface as follows: via the mDNS address "http://pipedal.local" (on Windows, Mac, or iPhone web 
browsers), at http://127.0.0.1 if you are interactively logged into your Raspberry Pi device, or at port 80 of the current network address of your 
Raspberry Pi, if you are connected from an Android device (which does not currently support mDNS).

To complete the initial configuration, you must either connect an Ethernet cable to your Raspberry pi so you can connect to the Web App (after which you should be able to connect to http://pipedal.local); or you must launch a web browser on your Raspberry pi device while logged in interactively. 

Once connected, select the Settings menu item on the Hamburger menu at the top left corner of the display. Click on Audio Device Settings to select and configure the audio device you want to use. 

You can also activate PiPedal's Wi-Fi hotspot connection from the Settings dialog. Click on the Wifi Hotspot menu item in the Settings dialog.

     IMPORTANT NOTE: Activating the WiFi hotspot will DISABLE outbound Wi-Fi connections from the Raspberry Pi.
     You will be able to access the PiPedal web interface through the hotspot connection, and make ssh and VNC 
     connections to the Raspberry Pi through the hotspot connection; but your Raspberry Pi will not have 
     outbound access to the Internet, unless an Ethernet cable is connected to the Raspberry Pi.

If you need access to the internet once the hotspot has been enabled, connect an Ethernet cable to
the Raspberry Pi. 

There are a number of other useful settings in the hamburger menu/Settings dialog. For example, most USB audio devices route instrument
input onto the right channel of the USB audio inputs. So you probably want to configure PiPedal to use only the right USB audio input channel. 
You can choose how to bind USB audio input and output channels (stereo, left only, right only) in the settings dialog. If you are using a Audio 
device that has more than two channels, you will be offered a list of channels to choose from instead.



## Command Line Configuration of PiPedal

The pipedalconfig program can be used to modify configuration of PiPedal from a shell command line. Run 'pipedalconfig --help' to view
available configuration commands, some of which are not avaialbe from the web interface. For example, you can change the port number
of the Web App HTTP server if you need to, using pipedalconfig.


## LV2 PLugins

PiPedal uses standard LV2 audio plugins for effects. There are currently a wide variety of available LV2 guitar effect plugins, and among 
these are a number of LV2 plugin collections that are specifically meant for use as guitar effects. 

Foremost among these is the Guitarix plugin collection: https://guitarix.org/. You should definitely install Guitarix:

      sudo apt install guitarix-lv2    

The Guitarix amp modelling plugins are on par with the best amp modelling effects anywhere.

Other highly recommended guitar effect collections:L
  
  sudo apt install zam-plugins    # http://www.zamaudio.com/
  
  sudo apt install mda-lv2  # http://drobilla.net/software/mda-lv2/
  
  sudo apt install calf-plugins  # http://calf-studio-gear.org/
  
  sudo apt install fomp  # http://drobilla.net/software/fomp/


But there are hundreds of other high-quality LV2 plugins that are suitable for use with PiPedal. 

Visit https://lv2plug.in/pages/projects.html for a more suggestions.

There is also a set of supplementary Gx effect plugins which did not make it into the main Guitarix distribution. You will
have to compile these plugins yourself, as they are not currently avaiable via apt. But if you are comfortable building
packages on Raspbian, the effort is well worthwhile. The GxPlugins collection provides a number of boutique amp emulations,
as well as emulations of famous distortion effect pedals.

- https://github.com/brummer10/GxPlugins.lv2


PiPedal will automatically detect installed LV2 plugins and make them selectable from the web app interface, as long as they meet the 
following conditions:

- Must have mono or stereo audio inputs and outputs.

- Must not be MIDI instruments or have CV (Control Voltage) inputs or outputs.

- Must be remotely controllable (no hard dependency on GUI-only controls), which is true of the vast majority of LV2 plugins).

Althouhg most LV2 plugins provide GUI interfaces, when running on a LINUX desktop, the LV2 plugin standard is specifically designed to allow remote control 
without using the provided desktop GUI interface. And all but a tiny minority of LV2 plugins support this.

## Building and Installing PiPedal

PiPedal has only been tested on Raspbian. But we pull requests to correct problems with building PiPedal on other versions of Linux are welcome.

To build PiPedal, a Raspberry Pi 4B, with at least 4GB of memory is recommended. You should be able to cross-compile PiPedal easily enough,
but we do not currently provide support for this. Consult CMake documentation on how to cross-compile source.

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

Run the following command to install and configure React dependencies.

   ./react-config   # Configure React dependencies.
  
### Building PiPedal

PiPedal was developed using Visual Studio Code. If you open the PiPedal project as a folder in VS Code, Code will 
detect the CMake configuration files, and automatically configure itself. Once the VS Code CMake plugin (written by Microsoft,
available through the plugins store) has configured itself, build commands should appear on the bottom line of Visual Studio 
Code. Visual Studio Code will take care of automatically configuring the project.

If you are not using Visual Studio Code, you can configure, build and install PiPedal using CMake build tools. For your convenience,
the following shell scripts have been provided in the root of the project in order to provide convenent CLI build commands.ka

    sudo ./init    # Configure the CMake build

    ./mk   # Build all targets.
    
    sudo ./install   # Deploy all targets 
    
    ./pkg    # Build a .deb file for distribution.
    
If you are using a development environment other than Visual Studio Code, it should be fairly straightforward to figure out how
to incorporate the PiPedal build procedure into your IDE workflow by examining the contents of the build shell scripts.npm

The CMake toolchain can be used to generate build scripts for other build environments as well (e.g. Visuual Studio .csproj files, Ant, or Linux
 Makefiles); and can be configured to perform cross-compiled builds as well. Consult documentation for CMake for instructions on how to
do that if you're interested. Visual Studio Code also provides quite painless procedures for cross-compiling CMake projects, and for 
remote building, and debugging. If you need to build for platforms other than Raspbian, or build on platforms other than Rasbian, 
you make want to investigate what Visual Studio Code's CMake integration provides in that regard.

### How to Debug PiPedal.


You must stop the pipdeal service before launching a debug instance of pipedald:

    sudo systemctl stop pipedald

or

    pipedalconfig -stop

But there's no harm in running a debug react server that's configured to connect to the web 
socket of a production instance of pipedal. 

PipPedal consists of the following subprojects:

*    A web application build in React, found in the react subdirectory.

*    `pipedald`: a Web server, written in C++, serving a web socket, and pre-built HTML components from the React app.
     All audio services are provided by the pipedal process.

*   `pipedalshutdownd`: A service to execute operations that require root credentials on behalf of pipedald. (e.g. shutdown, reboot,
    and pushing configuration changes.)

*   `pipedalconfig`: A CLI utility for managing and configuring the pipedald service.

In production, the pipedald web server serves the PiPedal web socket, as well as HTML from the  built 
react components. But while debugging, it is much more convenient to use the React debug server for 
React sources, and configure pipedald to serve only the websocket. 

To start the React debug server, from a shell, cd to the react directory, and run "./start". The react debug 
server will detect any changes to React sources, and rebuild them automatically (no build step required). 
Actual debugging is performed using the Chrome debugger (which is remarkably well integrated with React).

To get this to work on Raspberry Pi, you will probably have to make a configuration change.

Edit the file `/etc/sysctl.conf`, and add or increase the value for the maximum number of watchable user 
files:

    fs.inotify.max_user_watches=524288

followed by `sudo sysctl -p`. (VS Code and React both need this change).     

By default, the React app will attempt to contact the pipedal server on ws:*:8080 -- the address on which
the debug version of systemctld listens on. This can be reconfigured
in the file react/src/public/var/config.json if desired. (The pipedal server intercepts requests for this file and 
points the react app at itself in production, so the file has no effect when running in production). 
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

Once CMake has configured itself, build and debug commands are avaialble on the CMake toolbar at the 
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

You can avoid all of this, by configuring the debug instance to use a data folder in your home directory. Edit 
`debugConfig/config.json`:

    {
        ...
        "local_storage_path": "~/var/pipedal",
        ...
    }

