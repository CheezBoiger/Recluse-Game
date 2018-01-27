# Pretty Pictures
Current state of the Game Engine and it's graphics:

![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/Moonlight.png)
![alt tag](https://raw.githubusercontent.com/Cheezboiger/Recluse-Game/master/Regression/Shaders/ShadowsTest.png)
# How To Build
Building this application is actually a bit time consuming, but the end product should be to just run the game.

Recluse Engine requires the following SDK set up:

- Vulkan API (Rendering)
- Bullet3 (Physics)
- AudioKinetic WWise (Audio)
- Assimp (for model loading)

To Begin, you need to define these enviroment variables in order to link to the project:

- WWISESDK (For wwise directory)
- BULLETSDK (For Bullet, the root directory)

Furthurmore, you should have the following tools needed for build the project:

- CMake (3.0 or up)
- Visual Studio (64-bit build)

Bullet needs to be already compiled and ready to go (release is mainly used), as the project links to it's DLLs.
once done, simply create a directory and use cmake to build (be sure to use -G "Visual Studio 15 Win64" to build
x64 bit version of the product. 
