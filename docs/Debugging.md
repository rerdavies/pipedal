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

Although not strictly neccesary, you should probably add your login account to the pipedal_d group.
     
     sudo usermod -a -G pipedal_d *youruserid*

This will allow you to run `pipedald` under the debugger of your choice using the 
same /etc/pipedal and /var/pipedal directories as an instance of  `pipedal` running
under systemd. Note that when running under systemd, `pipedald` runs under an unprivileged 
`piedal_d` service account, and relies it's group (also 'pipedal_d`) in order to access its 
data files, and to communicate with the `pipedaladmin` service, which does run with root 
priveleges when `pipedald` needs to perform operations that do required root privileges. 

For what it's worth, `pipedaldmin` is virtually undebuggable, becuase it does require root privileges to run.
If you really _must_ debug pipedaladmin, you can fire up a sudo isntance of Visual Studio Code 
and attach to the running daemon process. But running  program as large as VS Code with root privileges
is a dangerous process, that VS Code firmly (and righfully) complains about. And configuring 
VSCode to run with root privileges as a painful process. Avoid if you can.

The pipedald service will run with or without the pipedaladmind service, but some operations (shutdown, reboot,
audio and Wi-Fi configuration changes) may fail if the pipedaladmind service is not running. Pipedal communicates 
with pipedaladmin via Unix docket that can only be opened by members of the pipedal_d group. So if you have 
added your own account to the `pipedal_d` group, debug instances of `pipedald` will in fact work properly.

In production, the pipedald web server serves the PiPedal web socket, as well as static HTML from the  built 
react components. But while debugging, it is much more convenient to use the React debug server for 
React sources, and configure pipedald to serve only the websocket. 

Note that a debug instance of `pipedald` cannot bind to port 80, since that requires either root privileges or 
access to port 80 via `authd`. So you will have to configure the debug isntance of `pipedald`'s web server to 
bind to port 8080 instead. The react server will serve the web applicaton on port 3000, so you will point your web 
browser to `raspberrpi:3000`. And you will then need to configure the react application to make web socket 
connection on port 8080 (where pipedald provides all dynamic content in the web app).

You may find it convenient to reconfigure the systemd intances of `pipedald` to bind to port 8080 as well. 
That will allow the react server to point clients to either a debug instance or the systemd instance of `pipedald`
depending, depending on which instance of pipedald is currently running. Run the following command to 
make the systemd instance of `pipedald` bind to port 8080 instead of port 80:

    pipedalconfig --install --port 8080

(which will also restart the `pipedald` service).

To start the React debug server, from a shell, `cd` to the react directory, and run `./start`. The react debug 
server will detect any changes to React sources, and rebuild them automatically (no build step required). 
Actual debugging is performed using the Chrome debugger (which is remarkably well integrated with React). 
You won't actually see changes to the version of the systemd version of the static web app until you 
do a full _Release_ or _RelWithDebInfo_ build of PiPedal, followed by running `./install.sh` which pushes 
the built react app in the location where the systemd version of `pipedald` serves static web content. 

By default, the debug React app will attempt to contact the pipedald server on ws:*:8080 -- the address on which
the debug version of pipedald listens on. This can be reconfigured 
in the file `react/src/public/var/config.json` if desired. If you connect to the the pipedald server port (port 80), pipedald intercepts requests for this file and  points the react app at itself, so the file has no effect when running in production. 

The React app will display the message "Error: Failed to connect to the server", until you start the pipedald websocket server in the VSCode debugger. However, it's quite reasonable to point the react debug app at a systemd instance of the pipedald server instead, if you don't intend to debug C++ code.

    react/public/var/config.json: 
    {
        ...
        "socket_server_port": 8080,  # (PiPedald's port number)
        "socket_server_address": "*",
        ...
    }

Setting socket_server_address to "*" configures Browers clients to make web socket connections on the same 
IP address they used to load the web app in the first place. If you set it to a specific address, the web app 
will attempt to establish a websocket connection son that specific address instead. (Not sure what that's useful
 for, but it's there if you ever need it). When running under systemd, Pipedal usually redirects browser clients 
 to an IPv4 address if the initial connection was made via IPv6, since long-lasting IPv6 websocket connections 
 are problematic, and IPv6 link-local adresses (which you sometimes get from mDNS servers) are completely toxic. 
 The React debug server does not do that (since the app configuration that clients get from the 
 React server is static). If you're connecting using mDNS (via 'raspberrypi:3000', for example), you could 
 end up connectiong to the React web app on an IPv6 address. So it might be useful for that. The easier solution 
 is probably just to use an explicit IPv4 address when you connect to the React debug server.

The original development for this app was done with Visual Studio Code. And it's probably easiest to go with the flow when building 
or debugging PiPedal. Open the root project directory in Visual Studio Code, and VSCode will automatically detect the CMake build 
files, and configure itself appropriately. It usually takes VSCode about 20 or 30 seconds to completely configure itself for a CMake project
and to settle down a bit. 

Once VSCode has configured itself, build and debug commands are available on the CMake toolbar at the 
bottom of the Visual Studio Code window. (Or in the CMake tab on the left-hand side of VSCOde if you have 
chosen not to configure the CMake add-on to make CMake controls visible on the bottom toolbar). 
Choose your compiler toolset. PipPedald sort of builds on a Clang toolset; although Clang may give warnings. But it's usually 
not significantly worse than building on a newly released version fo GCC.  It's probably best to do your first build with the build
variant set to RelWithDebugInfo. If you can get to the point that you can install Pipedal using ./install.sh, then that will 
ensure that all the various configuration files that Pipedal requires are deployed in all the right places, whether the services 
run properly or not. Check system logs using `journalctl -b0 | less` to see how well your newly built version of 
Pipedal is doing, and stop systemd services as neccesary and appropriate. 


Set the build variant to debug. Set the debug target to "pipedald".  Click on the Build button to build the app. Click on the Debug button to launch a debugger.

To get the debugger to launch and run correctly, you will need to set command-line parameters for pipedald. 
Command-line arguments can be set in the file `.vscode/launch.json`: 

        {
            "name": "(gdb) pipedald",
            "type": "cppdbg",
            "request": "launch",
            // Resolved by CMake Tools:
            "program": "${command:cmake.launchTargetPath}",
            "args": [   "/etc/pipedal/config",  "/etc/pipedal/react",  "-port", "0.0.0.0:8080", "-log-level","debug"  ],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [
                {
                    "name": "PATH",
                    "value": "$PATH:${command:cmake.launchTargetDirectory}"
                },
                {
                    "name": "LD_LIBRARY_PATH",
                    "value": "/usr/local/lib/aarch64-linux-gnu"
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

You must then set `pipedald` as the debug target on the CMake toolbar, AND configure the VSCode debugger 
to use the  `(gdb) pipedald` debug configuration. Click on the VSCode Debug tag on the left-hand side of the VSCode
window, and select the debug configuration using the dropdown found at the top edge of the Debug tab. 

The recommended way to debug `pipedald` is to point it at the `/etc/pipedal` and `/var/pipedal` directories 
that the live systemd version of `pipedald` uses. It is theoretically possible to get a debug instance to use 
a completely separate set of directories; but setting up the initial files and folders created by the PiPedal 
is complicated; and I have done that personally in a very long time. 


You will need to add your userid to the pipedal_d group if you plan to share the /var/pipedal directory. 
     
     sudo usermod -a -G pipedal_d *youruserid*

And you will need to reboot your machine to get the group membership change to take effect,or log out and log back
in if you can do that.

To debug the React app, use Chrome debugging tools. Load the PiPedal web app, and then press F12 in Chrome. You can 
then set breakpoints in PiPedal's Typescript source (visible on the Source Tab of the Chrome Debugger). Chrome provides 
very capable and functional debugging of Typescript right in the browser. 

There is a chrome add-on for debugging React apps; you may need to install the add-on in Chrome to get all of 
this to happen properly.

Having gone through a huge amount of work to get this point, let it be known that the Pipedal project is 
very receptive to accepting push request to its Github repository. So, if you have fixes, or changes, or enhancements
to a PiPedal fork, feel free to push them back to the main repository. The best way to do this is to create a branch on 
your personal fork of PiPedal, and make a pull request against the main PiPedal Github repository. Keeping your pushed changes
on a branch in your fork allows us to go back and forth a bit on proposed changes without making a mess of your fork. 


-----

[<< The Build System](TheBuildSystem.md) | [Up](Documentation.md)  | [PiPedal Architecture >>](Architecture.md)
