// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Core.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Win32/Keyboard.hpp"
#include "Core/Math/Quaternion.hpp"

#include "Game/Geometry/Cube.hpp"
#include "Game/Camera.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Physics/Physics.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Animation/Animation.hpp"
#include "Audio/Audio.hpp"
#include "UI/UI.hpp"



namespace Recluse {


// First person engine.
class Engine {
public:
  Engine();
  ~Engine();

  // Start up the engine with an initial window size.
  void      StartUp(std::string appName, i32 width, i32 height);
  void      CleanUp();

  Camera*   GetCamera() { return mCamera; }
  Window*   GetWindow() { return &mWindow; }

  void      Update(r64 dt);
  void      SetCamera(Camera* camera) { mCamera = camera; }

private:
  Camera*   mCamera;
  Window    mWindow;
};


// Global engine.
Engine&     gEngine();
} // Recluse