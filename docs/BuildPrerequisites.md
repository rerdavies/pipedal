### Build Prerequisites

Run the following commands to install build tools required by the PiPedal build.

    # install CMake
    sudo apt update
    sudo apt install cmake ninja-build

The PiPedal build process also requires version 12 or later of `node.js`. Type `node --version` to see if you have a version 
of `node.js` installed already. Otherwise run the following commands as root to install the v14.x lts version of `node.js`: 

   sudo apt install nodejs npm

If your distribution doesn't provide a suitable version of nodejs, you can install the current LTS version of nodejs
with

    # install NodeJS latest LTS release.
    curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
    sudo apt-get install -y nodejs


Run the following commands to install dependent libraries required by the PiPedal build.

    sudo apt update
    sudo apt install liblilv-dev libboost-dev libjack-jackd2-dev libnl-3-dev libnl-genl-3-dev libsystemd-dev catch
    sudo apt install libasound2-dev
    sudo apt install libwebsocketpp-dev authbind
    sudo apt install libsdbusc++-dev libsdbusc++-bin

Run the following command to install and configure React dependencies.

   ./react-config   # Configure React dependencies.
  
