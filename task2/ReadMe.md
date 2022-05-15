# Advanced Topic in Computer Graphics
## Getting started
This engine is based on SDL and OpenGL 4.0 to allow for as many platforms as possible to run it efficiently. We recommend that you use the engine on a Windows computer with a dedicated graphics card! It was also tested with internal graphics cards but your performance will be slow and you might encounter issues with debugging in the future.

## Build on Windows
We supply two batch files: build_debug.bat and build_release.bat to build the project. Just run them either directly by opening them/running them in a terminal, or by calling them via the IDE of your choice.
The batch file will search for a Visual Studio Installation (at least VS2017 with C++ build tools installed!) and uses the installed compiler to generate .exe files in the binaries folder.
Debug builds contain debug information and are built without compiler optimization (slow CPU code) and can be used to debug CPU code by opening the .exe file directly in Visual Studio. You can then just open a source file in VS and start debugging (setting breakpoints, inspecting variables etc.).

## Build on Mac
Mac setup can be a bit more involved. You need to install the required frameworks that are used by the engine:
 - SDL (https://www.libsdl.org/download-2.0.php)
 - GLEW (https://formulae.brew.sh/formula/glew)

For SDL you can directly download the framework from the page linked above and move it to your Frameworks folder. 
Glew needs to be installed using homebrew (https://brew.sh/). 
Depending on the version of your OS and some other factors you might need to adjust some paths in the supplied build_debug.command and build_release.command files. You should then be able to use them similarly to the batch file on Windows to build debug and release versions of the engine.

## Build on Linux
Linux received the least amount of testing but it should work. You might need to run following commands to install dependencies:

     $ sudo apt-get update
     $ sudo apt install g++
     $ sudo apt install mesa-common-dev
     $ sudo apt install libglu1-mesa-dev freeglut3-dev
     $ sudo apt install libsdl2-dev
     $ sudo apt install libglew-dev

Then you should be able to use build_debug.sh and build_release.sh to generate application binaries.