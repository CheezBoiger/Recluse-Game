# How To Build
Building this application is actually a bit time consuming, but the end product should be to just run the game.

Recluse Engine requires the following SDK set up:

- Vulkan API (Rendering)
- PhysX 3.4 or up (Physics)
- AudioKinetic WWise (Audio)

Furthurmore, you should have the following tools needed for build the project:

- CMake (3.0 or up)
- Visual Studio (64-bit build)

To Begin, you need to define these enviroment variables in order to link to the project:

- WWISESDK (For wwise directory)
- PHYSXSDK (For PhysX, the root directory)

PhysX needs to be already compiled and ready to go (both x64 debug and release), as the project links to it's DLLs.
once done, simply create a directory and use cmake to build (be sure to use -G "Visual Studio 15 Win64" to build
x64 bit version of the product. 