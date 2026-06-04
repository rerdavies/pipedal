### How to Debug PiPedal

PipPedal consists of the following components:

*    A web application built using vite/React, found in the `vite` project directory. The web application is built using the Vite build system as static content.

*   `pipedald`

    A web server, written in C++, serving a web socket, which serves both static content built by the 
    Vite build system, and dynamic content generated at runtime by the C++ code. The pipedald process also manages the audio services, and provides a web socket interface for the web application to control the audio services, and to get status information about the audio services.

*   `pipedaladmind`: 

    A service to execute operations that require root credentials on behalf of pipedald. (e.g. shutdown, reboot,
    and pushing configuration changes that require root credentials -- currently, configuration files for the 
    Wi-Fi hotspot which contain the Wi-Fi hotspot password).

*   `pipedalconfig`: 

    A CLI utility for managing and configuring the pipedald services.
     
*   `pipedaltest`: 

    Test cases for pipedald, built using the Catch2 framework.


You must stop the pipedald service before launching a debug instance of pipedald:

    sudo systemctl stop pipedald

or

    pipedalconfig --stop  
    
which also stops the pipedaladmind service.

You need to add your login account to the pipedal_d group.
     
     sudo usermod -a -G pipedal_d *youruserid*

This will allow you to run `pipedald` under the debugger of your choice using the 
same /etc/pipedal and /var/pipedal directories as an instance of  `pipedal` running
under systemd. Note that when running under systemd, `pipedald` runs under an unprivileged 
`pipedal_d` service account, and relies on its group (also `pipedal_d`) in order to access its 
data files, and to communicate with the privileged `pipedaladmin` service (which also requires membership in 
the `pipedal_d` group). The `pipedaladmin` service runs under with `root` credentials in order to perform 
operations that require root privileges (e.g. shutdown/reboot and updating the password for the Wi-Fi hotspot
connection). 

`pipedaladmin` is difficult to debug. The best way to do it is to start the service, and attach to the process in which it runs using the VSCode debugger. (Watch out for the prompt for root access that VSCode excentricly sends to the terminal window). 

The pipedald service will run with or without the pipedaladmind service, but some operations (shutdown, reboot,
audio and Wi-Fi configuration changes) may fail if the pipedaladmind service is not running. Pipedal communicates 
with pipedaladmin via Unix docket that can only be opened by members of the pipedal_d group. So if you have 
added your own account to the `pipedal_d` group, debug instances of `pipedald` will in fact work properly.

In production, the pipedald web server serves the PiPedal web socket, as well as static HTML from the built 
vite/React components. But while debugging, it is much more convenient to use the Vite debug server for 
React sources, and run a debug instance of `pipedald` to serve dyanmic content, and service websocket connections.

Note that a debug instance of `pipedald` cannot bind to port 80, since that requires either root privileges or 
access to port 80 via `authd`. So you will have to configure the debug instance of `pipedald`'s web server to 
bind to port 8080 instead. The react server will serve the web application on port 8080, so you will point your web 
browser to `localhost:8080`. And you will then need to configure the react application to make web socket 
connection on port 8080 (where pipedald provides all dynamic content in the web app). Note that the pipedald service 
serves the vite/react web app as compiled into /etc/pipedal/react directory, and the debug build of pipedal does not 
(by default) build the vite/react web app sources. Normally, you will use the vite/react debug server when 
you are debugging. However, when using the vite/react debug server, the PiPedal client application will 
use the pipedald web server on port 8080 to provide dynamic content (e.g. the web socket connect, and various
pieces of dynamic content served out of the `http://localhost:8080/var` URL and children thereof.

See the `sample_vscode_launch.json` file in the root project directory for an example of how to configure the VSCode 
debugger to launch a debug instance of `pipedald'. (Or just copy the entire file to `.vscode/launch.json`). If you are using another debugger, the invocation you will need for pipedald is: 

    pipedald /etc/pipedal/config /etc/pipedal/react -port 0.0.0.0:8080

which runs pipedald with the same configuration and react web app sources as the systemd instance of pipedald, but with the web server bound to port 8080 instead of port 80. Earlier debug procedures pointed the two supplied directories to locations that provided debug configurations and content. But it has subsequently proven easier just to ensure use the same configuration files and data as a systemd instance of `pipedald` would use. You will need to ensure that the systemd configuration directories have been properly provisioned with the necessary configuration files and data first. You can do that either by running `./install.sh` after a successful builde, or by installing a recent debian package for PiPedal downloaded from the Github releases page of https://githumb.com/rerdavies/pipedal .

You may find it convenient to reconfigure the systemd instance of `pipedald` to bind to port 8080 as well. 
That will allow the vite/react debug server to point clients to either a debug instance or the systemd instance of `pipedald`
depending, on which instance of pipedald is currently running. Run the following command to 
make the systemd instance of `pipedald` bind to port 8080 instead of port 80:

    pipedalconfig --install --port 8080

(which will also restart the `pipedald` service).

To start the Vite debug server, from a shell, `cd` to the `./vite` directory, and run `npm run dev` (or './debug.sh' which does the same thing. 

Vite debug server will automatically detect any changes to web app sources, and rebuild them automatically (no build step required). Note that the `pipedald` service must be running in order for the web app to function properly, either 
as a the `pipedald` service, or by running `pipedald` in a debugger.
Actual debugging of the web client is performed using the Chrome debugger (which is remarkably well integrated with Vite/React). 

You won't actually see changes to the version of the systemd version of the static web app until you 
do a full _Release_ or _RelWithDebInfo_ build of PiPedal, followed by running `./install.sh` which pushes 
the built react app in the location where the systemd version of `pipedald` serves static web content. 

However, you can refresh the Vite-generated content by running: 

```
cd ./vite
./build.sh   # Full build of the react app.
./install.sh  # Copy the built react app to the directory 
              # from which the systemd version of pipedal serves static content.
```

By default, the debug React app served by the Vite debug server will attempt to contact the pipedald server 
on ws:*:8080 -- the address on which the debug version of pipedald listens on. This can be reconfigured 
in the file `vite/vite.config.ts` if desired. Locate and edit the two URLS given in this section of the file:

vite/vite.config.ts:
```
  ...
  server: {
    proxy: {
      '/resources': {
        target: 'http://localhost:8080',
        changeOrigin: false,
      },
      '^/var/(?!config\\.json$).*': {
        target: 'http://localhost:8080',
        changeOrigin: false,
      },
    }
  }
  ...
```
The Vite debug server proxies requests to `http:/localhost:5173/var/*` and `http:/localhost:5173/resources/*` to the pipedald server on port 8080. The critical request from the web client is actually `http://localhost:5173/var/config.json`, the results of which contain, among other things, the address and port number of the `pipedald` web server. Once `config.json` has been downloaded, the client has the address it needs to make subsequent requests for non-static content and web socket connections directly to the `pipedald` web server instead of the Vite debug server. 

The React app will display the message "Error: Failed to connect to the server", until you start the pipedald web server  server in the VSCode debugger. However, it's quite reasonable to point the react debug app at a systemd instance of the pipedald server instead, if you don't intend to debug C++ code.

vite/vite.config.ts:
```
  ...
  server: {
    proxy: {
      '/resources': {
        target: 'http://localhost:80',
        changeOrigin: false,
      },
      '^/var/(?!config\\.json$).*': {
        target: 'http://localhost:80',
        changeOrigin: false,
      },
    }
  }
  ...
```


The original development for this app was done with Visual Studio Code. And it's probably easiest to go with the flow when building 
or debugging PiPedal. Open the root project directory in Visual Studio Code, and VSCode will automatically detect the CMake build 
files, and configure itself appropriately. It usually takes VSCode about 20 or 30 seconds to completely configure itself for a CMake project
and to settle down a bit. 

Once VSCode has configured itself, build and debug commands are available on the CMake toolbar at the 
bottom of the Visual Studio Code window. (Or in the CMake tab on the left-hand side of VSCOde if you have 
chosen not to configure the CMake add-on to make CMake controls visible on the bottom toolbar). 
Choose your compiler toolset. PiPedal will also build on a Clang toolsets, although that is a feature that gets tested infrequently, to be honest. It's probably best to do your first build with the build
variant set to RelWithDebugInfo. If you can get to the point that you can install Pipedal using ./install.sh, then that will 
ensure that all the various configuration files that Pipedal requires are deployed in all the right places, whether the services 
run properly or not. Check system logs using `journalctl -b0 | less` to see how well your newly built version of 
Pipedal is doing, and stop systemd services as necessary and appropriate.


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
