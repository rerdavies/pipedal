### How to Debug PiPedal.

PipPedal consists of the following components:

*    A web application build in React, found in the react subdirectory.

*   `pipedald`

    A web server, written in C++, serving a web socket, and pre-built HTML components from the React app.
    All audio services are provided by the pipedald process.

*   `pipedaladmind`: 

    A service to execute operations that require root credentials on behalf of pipedald. (e.g. shutdown, reboot,
    and pushing configuration changes).

*   `pipedalconfig`: 

    A CLI utility for managing and configuring the pipedald services.
     
*   `pipedaltest`: 

    Test cases for pipedald, built using the Catch2 framework.


You must stop the pipedald service before launching a debug instance of pipedald:

    sudo systemctl stop pipedald

or

    pipedalconfig -stop  #Stops the Jack service as well.

But there's no harm in running a debug react server that's configured to connect to the web 
socket of a production instance of pipedald on port 80, if you aren't planning to debug C++ code.

The pipedald service will run with or without the pipedaladmind service, but some operations (shutdown, reboot,
audio and Wi-Fi configuration changes) may fail if the pipedaladmind service is not running.

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

By default, the React app will attempt to contact the pipedald server on ws:*:8080 -- the address on which
the debug version of pipedald listens on. This can be reconfigured
in the file react/src/public/var/config.json if desired. If you connect to the the pipedald server port, pipedald intercepts requests for this file and 
points the react app at itself, so the file has no effect when running in production. 

The React app will display the message "Error: Failed to connect to the server", until you start the pipedald websocket server in the VSCode debugger. It's quite reasonable to point the react debug app at a production instance of the pipedald server instead.

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

To get the debugger to launch and run correctly, you will need to set command-line parameters for pipedald. 
Command-line arguments can be set in the file .vscode/settings.json: 

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

You will need to add your userid to the bluetooth group if you plan to debug Bluetooth onboarding code.

    sudo usermod -a -G bluetooth *youruserid*


Or you can avoid all of this, by configuring the debug instance to use a data folder in your home directory. Edit 
`debugConfig/config.json`:

    {
        ...
        "local_storage_path": "~/var/pipedal",
        ...
    }

-----
[<< The Build System](TheBuildSystem.md) | [Up](Documentation.md)