# Recluse Game and Engine Source Code
Current state of the Game Engine and it's graphics:

![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/TestGUI.png)
![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/ChromaticAbberrationTest.png)
![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/Helmet.png)
![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/PhysicsTest.png)
![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/Lantern.png)
# How To Build
Building this application is actually a bit time consuming, but the end product should be to just run the game.

Recluse Engine requires the following SDK set up:

- Vulkan API (Rendering)
- Bullet3 (Physics)
- AudioKinetic WWise (Audio)

To Begin, define these enviroment variables in order to link to the project:

- WWISESDK (For wwise directory)
- BULLETSDK (For Bullet, the root directory)

Furthurmore, the following tools are needed for building the project:

- CMake (3.0 or up)
- Visual Studio 2017 (64-bit build)

Bullet needs to be already compiled and ready to go (release and debug mode), as the project links to its static libraries.
Be sure to place the compiled libraries to the root directory, in a directory named "lib/Debug for debug, and lib/Release for release", 
of Bullet in order for Recluse CMake build to find them.

Once done, simply create a directory and use cmake to build (be sure to use -G "Visual Studio 15 Win64" to build
x64 bit version of the product. 
