### The Build System

PiPedal was developed using Visual Studio Code. Using Visual Studio Code to build PiPedal is recommended, but not required. The PiPedal
build procedure uses CMake project files, which are supported by most major Integrated Development Environments.
     
If you open the PiPedal project as a folder in VS Code, Code will 
detect the CMake configuration files, and automatically configure the project to use available toolchains. Once the VS Code CMake plugin (written by Microsoft,
available through the plugins store) has configured itself, build commands and configuration options should appear on the bottom line of Visual Studio 
Code. 

If you are not using Visual Studio Code, you can configure, build and install PiPedal using CMake build tools. For your convenience,
the following shell scripts have been provided in the root of the project in order to provide convenent CLI build commands.

    ./init    # Configure the CMake build (required if you change one of the CMakeList.txt files)

    ./mk   # Build all targets.
    
    sudo ./install   # Deploy all targets 
    
    ./pkg    # Build a .deb file for distribution.
    
If you are using a development environment other than Visual Studio Code, it should be fairly straightforward to figure out how
to incorporate the PiPedal build procedure into your IDE workflow by using the contents of the build shell scripts as a model.

The CMake toolchain can be used to generate build scripts for other build environments as well (e.g. Visual Studio .csproj files, Ant, or Linux
 Makefiles); and can be configured to perform cross-compiled builds as well. Consult documentation for CMake for instructions on how to
do that if you're interested. Visual Studio Code also provides quite painless procedures for cross-compiling CMake projects, and mostly-painless 
procedures for remote building, and/or debugging. If you need to build for platforms other than Raspbian, or build on platforms other than Rasbian, 
you may want to investigate what Visual Studio Code's CMake integration provides in that regard.
