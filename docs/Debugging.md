### How to Debug PiPedal

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

To start the React debug server, from a shell, `cd` to the react directory, and run `./start`. The react debug 
server will detect any changes to React sources, and rebuild them automatically (no build step required). 
Actual debugging is performed using the Chrome debugger (which is remarkably well integrated with React).

To get this to work on Raspberry Pi, you will probably have to make a configuration change.

Edit the file `/etc/sysctl.conf`, and add or increase the value for the maximum number of watchable user 
files:

    fs.inotify.max_user_watches=524288

followed by `sudo sysctl -p`. Note that VS Code and the React framework both need this change.

By default, the debug React app will attempt to contact the pipedald server on ws:*:8080 -- the address on which
the debug version of pipedald listens on. This can be reconfigured
in the file `react/src/public/var/config.json` if desired. If you connect to the the pipedald server port, pipedald intercepts requests for this file and  points the react app at itself, so the file has no effect when running in production. 

The React app will display the message "Error: Failed to connect to the server", until you start the pipedald websocket server in the VSCode debugger. It's quite reasonable to point the react debug app at a production instance of the pipedald server instead.

    react/public/var/config.json: 
    {
        ...
        "socket_server_port": 80,
        "socket_server_address": "*",
        ...
    }

Setting socket_server_address to "*" configures the web app to reconnect using the host address the browser
request used to connect to the web app. (e.g. 127.0.0.1, pipedal.local, the address of the Wi-Fi Direct connection &c). If you choose to provide an explicit address, remember that it is to that address that the web browser will connect.

The original development for this app was done with Visual Studio Code. Open the root project directory in
Visual Studio Code, and it will detect the CMake build files, and configure itself appropriately. Wait for 
the CMake plugin in Visual Studio Code to configure itself, after loading. 

Once CMake has configured itself, build and debug commands are available on the CMake toolbar at the 
bottom of the Visual Studio Code window. Set the build variant to debug. Set the debug target to "pipedald". 
Click on the Build button to build the app. Click on the Debug button to launch a debugger.

To get the debugger to launch and run correctly, you will need to set command-line parameters for pipedald. 
Command-line arguments can be set in the file `.vscode/launch.json`: 

    {
        ...
        "cmake.debugConfig": {
            "args": [
                "<projectdirectory>/debugConfig",
                "<projectdirectory>/build/react/src/build",
                "-port",
                "0.0.0.0:8080"
            ],

        ...
    }

where `<projectdirectory>` is the root directory of the pipedal project.

The default debug configuration for pipedal is configured to use /var/pipedal for storing working data files, 
which allows it to share configuration with a production instance of pipedal. Be warned that the permissioning 
for this folder is intricate. If you plan to use the data from a production server, get the production server 
installed first so the permissions are set correctly.
If you install a production instance later, remove the entire directory before doing so, to ensure that none 
of the files in that directory are permissioned incorrectly. 

You will need to add your userid to the pipedal_d group if you plan to share the /var/pipedal directory. 
     
     sudo usermod -a -G pipedal_d *youruserid*

You will need to reboot your machine to get the group membership change to take effect,or log out and log back
in if you can do that.

To run pipedald in a debugger, you need to use the following command-line to launch pipedal:

    build/src/pipedald /etc/pipedal/config /etc/pipedal/react -port 0.0.0.0:8080
    
The first argument specifies where the pipedal daemon's configuration files are. The second argument specifies where 
built static web pages for the web application are. And the port option specifies the port on which the daemon will
listen. If you are using a React debug server to serve up the web appliation you will point your browser at the 
React debug server on port 3000; but the web application will still need to connect to web sockets on the 8080 port.

If you are using Visual Studio Code, you might find it useful to add the following section to your `.vscode/launch.json` file:

    {
       ...
       {
            "name": "(gdb) pipedald",
            "type": "cppdbg",
            "request": "launch",
            // Resolved by CMake Tools:
            "program": "${command:cmake.launchTargetPath}",
            "args": [   "/etc/pipedal/config",  "/etc/pipedal/react",  "-port", "0.0.0.0:8080"  ],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [
                {
                    // add the directory where our target was built to the PATHs
                    // it gets resolved by CMake Tools:
                    "name": "PATH",
                    "value": "$PATH:${command:cmake.launchTargetDirectory}"
                }
            ],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        },
        ...
    }

-----

[<< The Build System](TheBuildSystem.md) | [Up](Documentation.md)  | [PiPedal Architecture >>](Architecture.md)
