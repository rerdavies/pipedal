### The Build System

PiPedal was developed using Visual Studio Code. Using Visual Studio Code to build PiPedal is recommended, but not required. The PiPedal
build procedure uses CMake project files, which are supported by most major Integrated Development Environments. You will probably 
need at least 8Gb of memory to use Visual Studio Code. You can probably build PiPedal from the command-line using 2Gb. If you have 
4Gb of memory, you may be able to build using Visual Studio Code remote compilation and remote debugging. If you try this, we would 
be grateful if you could share your experience on the [PiPedal discussion pages](https://github.com/rerdavies/pipedal/discussions).
     
If you open the PiPedal project as a folder in VS Code, Code will 
detect the CMake configuration files, and automatically configure the project to use available toolchains. Once the VS Code CMake plugin (written by Microsoft,
available through the Code plugins store) has configured itself, build commands and configuration options should appear on the bottom status line of Visual Studio Code. 

If you are not using Visual Studio Code, you can configure, build and install PiPedal using CMake build tools. For your convenience,
the following shell scripts have been provided in the root of the project.

    ./init    # Configure the CMake build (first time, or if you 
              # have changed one of the CMakeList.txt files)

    ./mk   # Build all targets.
    
    sudo ./install   # Deploy all targets 
    
    sudo ./makePackage    # Build a .deb file for distribution.
    
If you are using a development environment other than Visual Studio Code, it should be fairly straightforward to figure out how
to incorporate the PiPedal build procedure into your IDE workflow by using the contents of the build shell scripts as a model.

-----
[<< Build Prerequisites](BuildPrerequisites.md) | [Up](Documentation.md) | [How to Debug PiPedal >>](Debugging.md)