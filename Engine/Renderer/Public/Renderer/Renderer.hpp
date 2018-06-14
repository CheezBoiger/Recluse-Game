// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Types.hpp"
#include "Core/Win32/Window.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Thread/Threading.hpp"

#include "Resources.hpp"
#include "RenderQuad.hpp"
#include "GlobalDescriptor.hpp"
#include "LightDescriptor.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"

namespace Recluse {


class VulkanRHI;
class LightProbe;
class CommandBuffer;
class ReflectionProbe;
class GraphicsPipeline;
class ComputePipeline;
class GpuParams;
class GraphicsConfigParams;
class MeshData;
class MeshDescriptor;
class SkinnedMeshDescriptor;
class MaterialDescriptor;
class LightDescriptor;
class GlobalDescriptor;
class TextureCube;
class Semaphore;
class Texture1D;
class Texture2D;
class Texture3D;
class Texture2DArray;
class TextureCubeArray;
class TextureSampler;
class UIOverlay;
class Fence;
class RenderObject;
class SkinnedRenderObject;
class SkyRenderer;

// Renderer, which will be responsible for rendering out the scene from a
// camera's perspective. Renderer is a module in charge of drawing and displaying
// onto a window surface. This module is important as it is the only way to see 
// stuff on screen, and to display pretty graphics!
class Renderer : public EngineModule<Renderer> {
public:

  Renderer();
  ~Renderer();

  b32                Initialize(Window* window, const GraphicsConfigParams* params = nullptr);
  b32                Rendering() const { return m_Rendering; }

  // Configure the renderer, resulting either add/removing features of the renderer such as 
  // certain pipelines like shadowing, or quality of the display. Pass nullptr in order 
  // just plain recreate the renderer scene.
  // NOTE(): Only shadow disable/enable is allowed when changing shadow quality at runtime, otherwise you
  // need to restart the engine to change quality.
  void              UpdateRendererConfigs(const GraphicsConfigParams* params);

  // Clean up the renderer. This will "render" the renderer inactive.
  void              CleanUp();

  // Perform rendering of the display, you must call this function for each iteration in
  // the game loop!
  void              Render();

  // Callback used for EngineModule<> set up.
  void              OnStartUp() override;

  // Callback used for EngineModule<> set up.
  void              OnShutDown() override;

  // Builds/Updates commandbuffers for use in renderer. Very effective if you need to perform
  // a full update on the scene as a result of an application change, such as a window change. 
  // This will effectively stall the gpu if called too often. If you are adding/removing objects 
  // into a scene, use BuildAsync() instead.
  void              Build();
  
  // Builds the commandbuffers asyncronously, this will prevent stalling the gpu rendering process
  // by using temporary commandbuffers and building them instead. When done, they will replace old 
  // commandbuffers. Use only if you need to update the scene as a result of dynamic objects/materials being
  // added/removed to/from the scene. Do not use this call if there is a window change or 
  // application change, as it will result warnings from the renderer!
  void              BuildAsync();

  // Full RenderRHI wait til idle. This should not be called in time critical rendering.
  void              WaitIdle();

  // Creates a mesh object of which to submit to render.
  // Be sure to call FreeMeshData() if done with this mesh object.
  MeshData*         CreateMeshData();

  // Create a Mesh descriptor.
  MeshDescriptor*           CreateStaticMeshDescriptor();

  // Create a skinned mesh descriptor. This is used for skeletal animation.
  SkinnedMeshDescriptor*    CreateSkinnedMeshDescriptor();

  // Create a 1D texture.
  Texture1D*        CreateTexture1D();

  // Create a 2D Texture surface.
  Texture2D*        CreateTexture2D();

  // Create a 2D Array texture surface.
  Texture2DArray*   CreateTexture2DArray();

  TextureCubeArray* CreateTextureCubeArray();

  // Create a 3D texture surface.
  Texture3D*        CreateTexture3D();

  // Create a Texture Cube surface.
  TextureCube*      CreateTextureCube();

  // Create a Sampler.
  TextureSampler*   CreateTextureSampler();

  // Create a material descriptor.
  MaterialDescriptor* CreateMaterialDescriptor();

  GlobalBuffer*     GlobalData() { return m_pGlobal->Data(); }
  LightBuffer*      LightData() { return m_pLights->Data(); }

  // Frees up the allocated mesh data object.
  void              FreeMeshData(MeshData* mesh);

  //  Frees up the allocated texture1d object.
  void              FreeTexture1D(Texture1D* texture);

  // Frees up the allocated texture2d object.
  void              FreeTexture2D(Texture2D* texture);

  // Frees up the allocated texture2darray object.
  void              FreeTexture2DArray(Texture2DArray* texture);

  void              FreeTextureCubeArray(TextureCubeArray* texture);

  // Frees up the allocated texture3d object.
  void              FreeTexture3D(Texture3D* texture);

  // Frees up the cubemap texture object.
  void              FreeTextureCube(TextureCube* texture);

  // Frees up the texture sampler object.
  void              FreeTextureSampler(TextureSampler* sampler);

  // Frees up the material descriptor object.
  void              FreeMaterialDescriptor(MaterialDescriptor* descriptor);

  // Frees up mesh descriptor. Works for skinned mesh descriptor as well.
  void              FreeMeshDescriptor(MeshDescriptor* descriptor);

  // Offline enviroment cube map baking. This is used for the surrounding 
  // scene around the mesh surface we are rendering.
  TextureCube*      BakeEnvironmentMap(const Vector3& position);

  // Offline light probe baking. We can effectively then use this probe in the scene
  // to render our mesh object with fast global illumination. This generates an irradiance
  // mapped light probe.
  LightProbe*       BakeLightProbe(const TextureCube* envmap);

  // Offline reflections probe baking. We can effectively use this probe in the scene to render 
  // our mesh object. Generates a specular image based prefilter map.
  ReflectionProbe*  BakeReflectionProbe(const TextureCube* envmap);

  // Window reference.
  Window*           WindowRef() { return m_pWindow; }
  UIOverlay*        Overlay() { return m_pUI; }  

  // Check if this renderer is initialized with the window reference given.
  b32                Initialized() { return m_Initialized; }
  b32                Antialiasing() { return m_AntiAliasing; }

  // Get the rendering hardware interface used in this renderer.
  VulkanRHI*        RHI() { return m_pRhi; }
  SkyRenderer*      SkyRendererNative() { return m_pSky; }

  GlobalDescriptor* GlobalNative() { return m_pGlobal; }

  // Get current graphics configurations.
  GraphicsConfigParams& CurrentGraphicsConfigs() { return m_currentGraphicsConfigs; }

  // Enable HDR Post processing.
  void              EnableHDR(b32 enable);

  RenderQuad*       GetRenderQuad() { return &m_RenderQuad; }

  // Push mesh to render.
  void              PushMeshRender(MeshRenderCmd& cmd);

  const char*       GetDeviceName();

protected:
  // Start rendering onto a frame. This effectively querys for an available frame
  // to render onto.
  void              BeginFrame();

  // Once frame rendering is done, call this function to submit back to the swapchain 
  // for presenting to the window.
  void              EndFrame();

private:
  void              SetUpFrameBuffers();
  void              SetUpGraphicsPipelines();
  void              SetUpDescriptorSetLayouts();
  void              CleanUpDescriptorSetLayouts();
  void              CleanUpGraphicsPipelines();
  void              CleanUpFrameBuffers();
  void              CleanUpRenderTextures(b32 fullCleanup);
  void              CleanUpOffscreen(b32 fullCleanup);
  void              CleanUpFinalOutputs();
  void              SetUpFinalOutputs();
  void              SetUpForwardPBR();
  void              BuildForwardPBRCmdBuffer();
  void              SetUpRenderTextures(b32 fullSetup);
  void              SetUpOffscreen(b32 fullSetup);
  void              SetUpPBR();
  void              SetUpSkybox();
  void              BuildOffScreenBuffer(u32 cmdBufferIndex);
  void              BuildPbrCmdBuffer();
  void              BuildShadowCmdBuffer(u32 cmdBufferIndex);
  void              BuildHDRCmdBuffer();
  void              BuildSkyboxCmdBuffer();
  void              BuildFinalCmdBuffer();
  void              SetUpDownscale(b32 FullSetUp);
  void              CleanUpDownscale(b32 FullCleanUp);
  void              UpdateRuntimeConfigs(const GraphicsConfigParams* params);
  void              SetUpHDR(b32 fullSetup);
  void              CleanUpForwardPBR();
  void              CleanUpHDR(b32 fullCleanup);
  void              CleanUpPBR();
  void              CleanUpSkybox();
  void              UpdateSceneDescriptors();
  void              RenderOverlay();
  void              RenderPrimaryShadows();
  void              CheckCmdUpdate();
  inline u32        CurrentCmdBufferIdx() { return m_CurrCmdBufferIdx; }

  void              ClearCmdLists();
  void              SortCmdLists();
  void              WaitForCpuFence();

  Window*           m_pWindow;
  CmdList<MeshRenderCmd>            m_cmdDeferredList;
  CmdList<MeshRenderCmd>            m_forwardCmdList;
  CmdList<UiRenderCmd>              m_uiCmdList;
  GlobalDescriptor* m_pGlobal;
  LightDescriptor*  m_pLights;

  // NOTE(): This can be abstracted, but we will be tight coupling with Vulkan anyway...
  VulkanRHI*        m_pRhi;

  struct {
    std::vector<CommandBuffer*>   _CmdBuffers;
    std::vector<CommandBuffer*>   _ShadowCmdBuffers;
    Semaphore*                    _Semaphore;
  } m_Offscreen; 

  struct {
    CommandBuffer*                _CmdBuffer;
    Semaphore*                    _Sema;
  } m_Pbr;

  struct {
    CommandBuffer*                _CmdBuffer;
    Semaphore*                    _Semaphore;
  } m_Forward;

  struct {
    CommandBuffer*                _CmdBuffer;
    Semaphore*                    _Semaphore;
    b8                            _Enabled;
  } m_HDR;

  struct {
    i32                           _Horizontal;
    r32                           _Strength;
    r32                           _Scale;
  } m_Downscale;

  CommandBuffer*        m_pSkyboxCmdBuffer;
  CommandBuffer*        m_pFinalCommandBuffer;
  Fence*                m_cpuFence;
  Semaphore*            m_pFinalFinished;
  Semaphore*            m_SkyboxFinished;
  RenderQuad            m_RenderQuad;
  GraphicsConfigParams  m_currentGraphicsConfigs;
  UIOverlay*            m_pUI;
  SkyRenderer*          m_pSky;
  u32                   m_CurrCmdBufferIdx;

  u32                   m_TotalCmdBuffers;

  b32                   m_Rendering : 1;
  b32                   m_Initialized : 1;
  b32                   m_AntiAliasing : 1; 
  b32                   m_Minimized : 1;
};

Renderer&           gRenderer();
} // Recluse