### Build Prerequisites

Run the following commands to install build tools required by the PiPedal build.

    # install CMake
    sudo apt updatee
    sudo apt install -y cmake ninja-build build-essential g++ git

The PiPedal build process also requires version 12 or later of `node.js`. Type `node --version` to see if you have a version 
of `node.js` installed already. Otherwise run the following commands as root to install the v14.x lts version of `node.js`: 
`
   sudo apt install nodejs npm curl

If your distribution doesn't provide a suitable version of nodejs, you can install the current LTS version of nodejs
with

    # install NodeJS latest LTS release.
    curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
    sudo apt-get install -y nodejs npm


Run the following commands to install dependent libraries required by the PiPedal build.
 
    sudo apt update 
    sudo apt install -y liblilv-dev libboost-dev  \
        libsystemd-dev catch libasound2-dev uuid-dev \
        authbind libavahi-client-dev  libnm-dev libicu-dev \
        libsdbus-c++-dev libzip-dev google-perftools \
        libgoogle-perftools-dev
    

### Installing Sources

If you are using Visual Studio Code, install the following Extensions:

- C/C++ Extension Pack (Microsoft).
- CMake Tools (Microsoft)

Clone the rerdavies/pipdal package from github. Use the source control tools from Visual Studio Code, or 
   
    cd ~/src    # or whereever you keep your source repositories.
    git clone https://github.com/rerdavies/pipedal.git 
   
In the project root, run the following commands to initialze and update pipedal submodules. These steps 
must be performed even if you used Visual Studio Code to initially install the project.

    cd ~/src/pipedal
    git submodule update --init --recursive
   
Run the following command to install and configure React dependencies.

    cd ~/src/pipedal
    ./react-config   # Configure React NPM dependencies.

And one final step. Edit the file `/etc/sysctl.conf`, and add or increase the value for the maximum number of watchable user 
files:

    fs.inotify.max_user_watches=524288

Then run `sudo sysctl -p` to get the change to take effect. Visual Studio Code and the React Debug server both need this 
setting to run properly. Older versions of Raspberry Pi OS set this value too low; and I am not honestly sure whether current 
versions of Raspberry Pi OS have fixed the problem.
   
--------------------------   
[<< Building PiPedal from Source](BuildingPiPedalFromSource.md) | [Up](Documentation.md) | [The Build System >>](TheBuildSystem.md)
 
