// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Types.hpp"
#include "Core/Win32/Window.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Thread/Threading.hpp"

#include "Resources.hpp"
#include "ScreenQuad.hpp"

namespace Recluse {


class VulkanRHI;
class CmdList;
class DirectionLight;
class PointLight;
class SpotLight;
class LightProbe;
class CommandBuffer;
class ReflectionProbe;
class GraphicsPipeline;
class ComputePipeline;
class GpuParams;
class UserParams;
class Mesh;
class Material;
class LightMaterial;
class GlobalMaterial;
class CubeMap;
class Semaphore;


// Renderer, which will be responsible for rendering out the scene from a
// camera's perspective.
class Renderer : public EngineModule<Renderer> {
public:
  // Definition of the UI Overlay for which to render onto.
  struct UIOverlay {
    
  protected:
    VulkanRHI*                  mRhiRef;

    void                        Initialize(VulkanRHI* rhi);
    void                        CleanUp();
    void                        Render();
    std::vector<CommandBuffer*> cmdBuffers;
    friend Renderer;
  };

  Renderer();
  ~Renderer();

  b8                Initialize(Window* window);
  b8                Rendering() const { return mRendering; }
  void              Configure(UserParams* params);
  void              UpdateFromWindowChange();
  void              CleanUp();
  void              Render();

  void              OnStartUp() override;
  void              OnShutDown() override;
  void              PushCmdList(CmdList* cmdList) { mCmdList = cmdList; }
  void              PushDeferredCmdList(CmdList* cmdList) { mDeferredCmdList = cmdList; }
  void              Build();
  void              BuildAsync();

  void              SetGlobalMaterial(GlobalMaterial* material) { mGlobalMat = material; }
  void              SetLightMaterial(LightMaterial*   material) { mLightMat = material; }

  void              BeginFrame();
  void              EndFrame();

  Mesh*             CreateMesh();
  Material*         CreateMaterial();
  GlobalMaterial*   CreateGlobalMaterial();
  LightMaterial*    CreateLightMaterial();
  DirectionLight*   CreateDirectionLight();
  PointLight*       CreatePointLight();
  SpotLight*        CreateSpotLight();

  CubeMap*          BakeEnvironmentMap(const Vector3& position);
  LightProbe*       BakeLightProbe(const CubeMap* envmap);
  Window*           WindowRef() { return mWindowHandle; }
  UIOverlay*        Overlay() { return &mUI; }  

private:
  void              SetUpFrameBuffers();
  void              SetUpGraphicsPipelines();
  void              CleanUpGraphicsPipelines();
  void              CleanUpFrameBuffers();
  void              CleanUpRenderTextures();
  void              CleanUpOffscreen();
  void              SetUpRenderTextures();
  void              SetUpOffscreen();
  void              UpdateMaterials();
  void              RenderOverlay();

  Window*           mWindowHandle;
  CmdList*          mCmdList;
  CmdList*          mDeferredCmdList;
  GlobalMaterial*   mGlobalMat;
  LightMaterial*    mLightMat;

  // NOTE(): This can be abstracted, but we will be tight coupling with Vulkan anyway...
  VulkanRHI*        mRhi;

  struct {
    CommandBuffer*  cmdBuffer;
    Semaphore*      semaphore;
  } mOffscreen; 

  ScreenQuad        mScreenQuad;
  UIOverlay         mUI;
  b8                mRendering;
};

Renderer&           gRenderer();
} // Recluse