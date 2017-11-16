// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Core.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Win32/Keyboard.hpp"
#include "Core/Math/Quaternion.hpp"

#include "Geometry/Cube.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "Core/Logging/Log.hpp"

#include "Renderer/Renderer.hpp"
#include "Renderer/Material.hpp"
#include "Renderer/RenderCmd.hpp"
#include "Renderer/CmdList.hpp"

#include "Physics/Physics.hpp"
#include "Filesystem/Filesystem.hpp"
#include "Animation/Animation.hpp"
#include "Audio/Audio.hpp"
#include "UI/UI.hpp"



namespace Recluse {


// Scene graph to push into the engine.
class Scene;


// First person engine.
class Engine {
public:
  Engine();
  ~Engine();

  // Start up the engine with an initial window size. As the window is initialized 
  // along with the start up, be sure to manually call the Show() function from the 
  // window in order to see something!
  void                          StartUp(std::string appName, b8 fullscreen, i32 width = 800, i32 height = 600);
  void                          CleanUp();
  void                          ProcessInput();

  Camera*                       GetCamera() { return mCamera; }
  Window*                       GetWindow() { return &mWindow; }

  void                          Update(r64 dt);
  void                          SetCamera(Camera* camera) { mCamera = camera; }

  // Push the new scene to into this engine for extraction.
  void                          PushScene(Scene* scene);
  void                          LoadSceneTransition();

  LightBuffer*                  LightData() { return mLightMat->Data(); }
  GlobalBuffer*                 GlobalData() { return mCamMat->Data(); }

  CmdList&                      RenderCommandList() { return mRenderCmdList; }

  // TODO(): When new scene changes, we need to rebuild our commandbuffers in the 
  // renderer. This will need to be done by swapping old light material with new and 
  // rebuilding...

private:
  GlobalMaterial*               mCamMat;
  LightMaterial*                mLightMat;
  Camera*                       mCamera;
  Scene*                        mPushedScene;

  Window                        mWindow;
  CmdList                       mRenderCmdList;
  CmdList                       mDeferredCmdList;
};


// Global engine.
Engine&     gEngine();
} // Recluse