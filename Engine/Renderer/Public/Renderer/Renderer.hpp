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
  void              UpdateRendererConfigs(UserParams* params);
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
  void              WaitIdle();

  // Creates a mesh object of which to submit to render.
  // Be sure to call FreeMesh() if done with this mesh object.
  Mesh*             CreateMesh();

  // Creates a material object to submit material samples of which to render a
  // Mesh. Be sure to call FreeMaterial() if done with this material object.
  Material*         CreateMaterial();

  GlobalMaterial*   CreateGlobalMaterial();
  LightMaterial*    CreateLightMaterial();
  DirectionLight*   CreateDirectionLight();
  PointLight*       CreatePointLight();
  SpotLight*        CreateSpotLight();

  // Frees up the allocated mesh object.
  void              FreeMesh(Mesh* mesh);

  // Frees up the allocated material object.
  void              FreeMaterial(Material* material);

  // Frees up the allocated global material object.
  void              FreeGlobalMaterial(GlobalMaterial* material);

  // Frees up the allocated light material object.
  void              FreeLightMaterial(LightMaterial* material);

  // Offline enviroment cube map baking. This is used for the surrounding 
  // scene around the mesh surface we are rendering.
  CubeMap*          BakeEnvironmentMap(const Vector3& position);

  // Offline light probe baking. We can effectively then use this probe in the scene
  // to render our mesh object with fast global illumination.
  LightProbe*       BakeLightProbe(const CubeMap* envmap);

  // Window reference.
  Window*           WindowRef() { return mWindowHandle; }
  UIOverlay*        Overlay() { return &mUI; }  

  // Check if this renderer is initialized with the window reference given.
  b8                Initialized() { return mInitialized; }

  // Get the rendering hardware interface used in this renderer.
  VulkanRHI*        RHI() { return mRhi; }

private:
  void              SetUpFrameBuffers();
  void              SetUpGraphicsPipelines();
  void              SetUpDescriptorSets();
  void              CleanUpDescriptorSets();
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
  b8                mInitialized;
};

Renderer&           gRenderer();
} // Recluse