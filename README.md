

### How to Debug PiPedal.###


You must stop the pipdeal service before launching a debug instance of pipedal:

>   sudo systemctl stop pipedal
or

>   pipedalconfig -stop

But there's no harm in running a debug react server that's configured to connect to the web 
socket of a production instance of pipedal. 

PipPedal consists of the following subprojects:

*    A web application build in React. 

*    pipedald: a Web server, written in C++, serving a web socket, and pre-built HTML components from the React app.
     All audio services are provided by the pipedal process.

*   pipedalshutdownd: A service to execute operations that require root credentials on behalf of pipedald. (e.g. shutdown, reboot,
    and pushing configuration changes.)

In production, the pipedald web server serves the PiPedal web socket, as well as HTML from the  built 
react components. While debugging, it is much more convenient to use the React debug server for 
React sources, and configure pipedald to serve only the websocket. 

To start the React debug server, from a shell, cd to the react directory, and run "./start". The react debug 
server will detect any changes to React sources, and rebuild them automatically (no build step required). 
Actual debugging is performed using the Chrome debugger (which is remarkably well integrated with React).

To get this to work on Raspberry Pi, you will probably have to make a configuration change.

Edit the file '/etc/sysctl.conf', and add or increase the value for the maximum number of watchable user 
files:
     fs.inotify.max_user_watches=524288

followed by 'sudo sysctl -p'. (VS Code and React both need this change).     

By default, the React app will attempt to contact the pipedal server on ws:*:8080. This can be reconfigured
in the file react/src/public/var/config.json. (The pipedal server intercepts requests for this file and 
points the react app at itself in production). The React app will display the message 
"Error: Failed to connect to the server", until you start the pipedal websocket server in the VSCode debugger.
It's quite reasonable to point the react debug app at a production instance of the pipedal server.

>    react/src/public/var/config.json: 
>    {
>        "socket_server_port": 80,
>        "socket_server_address": "*",
>        ...>
>    }

Setting socket_server_address to "*" configures the web app to reconnect using the host address the browser
request used to connect to the web app. (e.g. 127.0.0.1, or pipedal.local, &c). If you choose another address,
remember that it is that web browser which will use it to make the connection.

The original development for this app was done with Visual Studio Code. Open the root project directory in
Visual Studio Code, and it will detect the CMake build files, and configure itself appropriately. Wait for 
the CMake plugin in Visual Studio Code to configure itself, after loading. 

Once CMake has configured itself, build and debug commands are avaialble on the CMake toolbar at the 
bottom of the Visual Studio Code window. Set the build variant to debug. Set the debug target to "pipedal". 
Click on the Build button to build the app. Click on the Debug button to launch a debugger.

To get the debugger to launch and run correctly, you will need to set commandline parameters for pipedal. 
Commandline arguments can be set in the file .vscode/settings.json: 

{
    ...
    "cmake.debugConfig": {
        "args": [
          "<projectdiretory>/debugConfig","<projectdiretory>/build/react/src/build",  "-port", "0.0.0.0:8080"
        ],

    ...
}

where <projectdirectory> is the root directory of the pipedal project.

The default debug configuration for pipedal is configured to use /var/pipedal for storing working data files, 
which allows it to share configuration with a production instance of pipedal. Be warned that the permissioning 
for this folder is intricate. If you plan to use the data from a production server, get the production server 
installed first so the permissions are set correctly. If you install a production instance later, remove the 
entire directory before doing so, to ensure that none of the files in that directory are permissioned 
incorrectly. 

You will need to add your userid to the pipedal_d group if you plan to share the /var/pipedal directory. 

You can avoid all of this, by configuring the debug instance to use a data folder in your home directory. Edit 
debugConfig/config.json:

> {
>    ...
>    "local_storage_path": "~/var/pipedal",
>    ...
> }

