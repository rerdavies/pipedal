# PiPedal

PiPedal is a multi-effect guitar pedal that provides a clean compact web interface that's suitable for use on small-format devices 
like phones and tablets, although it works gloriously with desktop browers as well.

PiPedal uses LV2 audio plugins. There are currently a wide variety of sources for excellent LV2 guitar plugins. You can add as many 
plugins to your patch as your CPU will support (well over a dozen on a Raspberry Pi 4+). Audio signal chains
can have an arbitrary number of split chains, which maybe A/B-selected or mixed if you wish.

If you have a USB Midi Controller, you can bind Midi controls and notes to LV2 plugin controls using the Midi Bindings dialog.
This particularly useful if you have a USB Midi footccontroller or a small USB Midi controller such as the Korg Nano or AKAI \
midi controllers.

## Configuring PiPedal After Installation

After PiPedal is installed, you can connect to the web interface as follows: via the mDNS address "http://pipedal.local" (on Windows, Mac, or iPhone web 
browsers), at http://127.0.0.1 if you are interactively logged into your Raspberry Pi device, or at port 80 of the current network address of your 
Raspberry Pi, if you are connected from an Android device (which does not currently support mDNS).

To complete the initial configuration, you must either connect an Ethernet cable to your Raspberry pi so you can connect to the Web App (after which you should be able to connect to http://pipedal.local); or you must launch a web browser on your Raspberry pi device while logged in interactively. 

Once connected, you can activate a WiFi hotspot on your Rasberry Pi which can be used to connect to PiPedal while it is not connected to an Ethernet network. 

     IMPORTANT NOTE: Activating the WiFi hotspot will DISABLE outbound Wi-Fi connections from the Raspberry Pi.
     You will be able to access the PiPedal web interface through the hotspot connection, and make ssh and VNC 
     connections to the Raspberry Pi through the hotspot connection; but your Raspberry Pi will not have outbound access 
     to the Internet. 

If you need access to the internet once the hotspot has been enabled, connect an Ethernet cable to
the Raspberry Pi. 

Settings to configure the WiFi hotspot are available in the hamburger menu/Settings dialog.

There are a number of other useful settings in the hamburger menu/Settings dialog. For example, most USB audio devices route instrument
input onto the right channel of the USB audio inputs. So you probably want to configure PiPedal to use only the right USB audio input channel.

You can choose how to bind USB audio input and output channels (stereo, left only, right only) in the settings dialog. 

## Command Line Configuration of PiPedal

The pipedalconfig program can be used to modify configuration of PiPedal from a shell command line. Run 'pipedalconfig --help' to view
available configuration commands, some of which are not avaialbe from the web interface. (For example, you can change the port number
of the Web App HTTP server if you need to, uusing pipedalconfig).


## LV2 PLugins

PiPedal uses standard LV2 audio plugins for effects. There are currently a wide variety of available LV2 guitar effect plugins, foremost of
which is the Guitarix plugin collection: https://guitarix.org/

To get started, install the Guitarix LV2 plugin collection.

  sudo apt install jackd 
  sudo apt install guitarix-lv2
  sudo apt install zam-plugins
  sudo apt install mda-lv2
  sudo apt install fomp

But there are hundreds of other high-quality LV2 plugins that are suitable for use with PiPedal. 

 - Zam Plugins. http://www.zamaudio.com/

 - Calf Plugins. http://calf-studio-gear.org/
 
Or visit https://lv2plug.in/pages/projects.html for a more complete list. 

There is also a set of supplementary Gx effect plugins which did not make it into the main Guitarix distribution. You
have to build these plugins yourself, but the effort is well worthwhile.

- https://github.com/brummer10/GxPlugins.lv2

PiPedal will automatically detect installed LV2 plugins and make them selectable from the web app interface, as long as they meet the 
following conditions:

- Must have mono or stereo audio inputs and outputs.

- Must not have MIDI or CV inputs.

- Must be remotely controllable (no hard dependency on GUI-only controls), which is true of the vast majority of LV2 plugins.

Most LV2 plugins have GUI interfaces, which are not used by PiPedal; but the LV2 plugin standard allows almost all LV2 plugins to 
be controlled via PiPedal's web interface.

### Building and Installing PiPedal

PiPedal has only been tested on Raspbian. But we will gladly accept pull requests to correct problems with other versions of Linux.

To build PiPedal, a Raspberry Pi 4B, with at least 4GB of memory is recommended. You should be able to cross-compile PiPedal easily enough,
but we do not currently provide support for this. Consult CMake documentation on how to cross-compile source.

Run the following commands to install build tools required by the PiPedal build.

    # install CMake
    sudo apt update
    sudo apt install cmake 

    # install NodeJS lastest LTS release.
    curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
    sudo apt-get install -y nodejs


Run the following commands to install dependent components required by the PiPedal build.

    sudo apt update
    sudo apt install liblilv-dev libboost-dev libjack-jackd2-dev libnl-3-dev libnl-genl-3-dev libsystemd-dev catch

The PiPedal build process also requires version 14 or later of `node.js`. Type `node --version` to see if you have a version 
of `node.js` installed already. Otherwise run the following commands as root to install the v14.x lts version of `node.js`: 

    curl -fsSL https://deb.nodesource.com/setup_lts.x | bash -
    apt-get install -y nodejs


### Building PiPedal.ini

PiPedal was developed using Visual Studio Code. If you open the PiPedal project as a folder in VS Code, Code will 
detect the CMake configuration files, and automatically configure itself. Once the VS Code CMake plugin has configured
itself, build commands should appear on the bottom line of the Visual Studio Code interface. 

To install PiPedal, run a full non-debug build using Visual Studio Code, and then run the following command 
in the root project directory:

    sudo ,./init

If you are using a different development environment, you can build the project by running the following 
command in the root project directory.

    ./mk

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

