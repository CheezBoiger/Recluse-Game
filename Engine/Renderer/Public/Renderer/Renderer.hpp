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
#include "Particles.hpp"
#include "CmdList.hpp"
#include "RenderCmd.hpp"
#include "HDR.hpp"


namespace Recluse {


class VulkanRHI;
class CommandBuffer;
class ReflectionProbe;
class GraphicsPipeline;
class ComputePipeline;
class GpuParams;
class GraphicsConfigParams;
class MeshData;
class MeshDescriptor;
class JointDescriptor;
class MaterialDescriptor;
class LightDescriptor;
class GlobalDescriptor;
class GlobalIllumination;
class UIDescriptor;
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
class DebugManager;
class RenderObject;
class SkinnedRenderObject;
class AntiAliasingFXAA;
class SkyRenderer;
class DecalEngine;
class ParticleEngine;
class HDR;
class Clusterer;
class BakeIBL;

struct SamplerInfo;
struct LightProbe;

typedef U32 renderer_key_t;

// Renderer, which will be responsible for rendering out the scene from a
// camera's perspective. Renderer is a module in charge of drawing and displaying
// onto a window surface. This module is important as it is the only way to see 
// stuff on screen, and to display pretty graphics!
class Renderer : public EngineModule<Renderer> {
  // Maximum number of workers allowed in renderer for multithreaded rendering.
  static const U32  kMaxRendererThreadWorkerCount = 3;
  static const char* appName;
public:

  Renderer();
  ~Renderer();

  B32                initialize(Window* window, const GraphicsConfigParams* params = nullptr);
  B32                isRendering() const { return m_Rendering; }

  void              setAppName(const char* name) { appName = name; }

  void              takeSnapshot(const std::string screenshotname);
  void              setHardwareHints(const U32 rhiBits) { m_rhiBits = rhiBits; }

  // Configure the renderer, resulting either add/removing features of the renderer such as 
  // certain pipelines like shadowing, or quality of the display. Pass nullptr in order 
  // just plain recreate the renderer scene.
  // NOTE(): Only shadow disable/enable is allowed when changing shadow quality at runtime, otherwise you
  // need to restart the engine to change quality.
  void              updateRendererConfigs(const GraphicsConfigParams* params);

  // Clean up the renderer. This will "render" the renderer inactive.
  void              cleanUp();

  // Perform rendering of the display, you must call this function for each iteration in
  // the game loop!
  void              render();

  // Callback used for EngineModule<> set up.
  void              onStartUp() override;

  // Callback used for EngineModule<> set up.
  void              onShutDown() override;

  // Full RenderRHI wait til idle. This should not be called in time critical rendering.
  void              waitIdle();

  // Create a Mesh descriptor.
  MeshDescriptor*           createMeshDescriptor();

  // Create a joint descriptor for skinned meshes. This is used for skeletal animation.
  JointDescriptor*    createJointDescriptor();

  // Create a 1D texture.
  Texture1D*        createTexture1D();

  // Create a 2D Texture surface.
  Texture2D*        createTexture2D();

  // Create a 2D Array texture surface.
  Texture2DArray*   createTexture2DArray();

  TextureCubeArray* CreateTextureCubeArray();

  // Create a 3D texture surface.
  Texture3D*        createTexture3D();

  // Create a Texture Cube surface.
  TextureCube*      createTextureCube();

  // Create a Sampler.
  TextureSampler*   createTextureSampler(const SamplerInfo& info);

  // Create a material descriptor.
  MaterialDescriptor* createMaterialDescriptor();

  ParticleSystem* createParticleSystem(U32 maxInitParticleCount = 100);

  // Create a UI descriptor.
  UIDescriptor*     createUIDescriptor();

  GlobalBuffer*     getGlobalData() { return m_pGlobal->getData(); }
  LightBuffer*      getLightData() { return m_pLights->getData(); }

  void              freeParticleSystem(ParticleSystem* system);

  //  Frees up the allocated texture1d object.
  void              freeTexture1D(Texture1D* texture);

  // Frees up the allocated texture2d object.
  void              freeTexture2D(Texture2D* texture);

  // Frees up the allocated texture2darray object.
  void              freeTexture2DArray(Texture2DArray* texture);

  void              freeTextureCubeArray(TextureCubeArray* texture);

  // Frees up the allocated texture3d object.
  void              freeTexture3D(Texture3D* texture);

  // Frees up the cubemap texture object.
  void              freeTextureCube(TextureCube* texture);

  // Frees up the texture sampler object.
  void              freeTextureSampler(TextureSampler* sampler);

  // Frees up the material descriptor object.
  void              freeMaterialDescriptor(MaterialDescriptor* descriptor);

  // Frees up mesh descriptor objects.
  void              freeMeshDescriptor(MeshDescriptor* descriptor);

  // Frees up joint descriptor objects.
  void              freeJointDescriptor(JointDescriptor* descriptor);

  // Frees up the UI descriptor object.
  void              freeUIDescriptor(UIDescriptor* descriptor);

  // Offline enviroment cube map baking. This is used for the surrounding 
  // scene around the mesh surface we are rendering.
  // Takes the position of where to bake the cubemap in, along with the texSize of the cubemap
  // dimensions for each texture surface. When Calling this function, will render all objects currently
  // in the render queue.
  TextureCube*      bakeEnvironmentMap(const Vector3& position, U32 texSize = 512u);

  // Offline light probe baking. We can effectively then use this probe in the scene
  // to render our mesh object with fast global illumination. This generates an irradiance
  // mapped light probe.
  LightProbe*       bakeLightProbe(const TextureCube* envmap);

  // Offline reflections probe baking. We can effectively use this probe in the scene to render 
  // our mesh object. Generates a specular image based prefilter map.
  ReflectionProbe*  bakeReflectionProbe(const TextureCube* envmap);

  // Generate a BRDF LUT with given lookup size. Defaults to 512x512.
  Texture2D*        generateBRDFLUT(U32 x = 512u, U32 y = 512u);

  // Window reference.
  Window*           getWindowRef() { return m_pWindow; }
  UIOverlay*        getOverlay() { return m_pUI; }  

  // Check if this renderer is initialized with the window reference given.
  B32  isInitialized() { return m_Initialized; }
  B32  usingAntialiasing() { return m_AntiAliasing; }

  // Get the rendering hardware interface used in this renderer.
  VulkanRHI* getRHI() { return m_pRhi; }
  SkyRenderer* getSkyRendererNative() { return m_pSky; }

  GlobalDescriptor* getGlobalNative() { return m_pGlobal; }

  // Get current graphics configurations.
  GraphicsConfigParams& getCurrentGraphicsConfigs() { return m_currentGraphicsConfigs; }

  // setEnable HDR Post processing.
  void enableHDR(B32 enable);

  // Get this renderer's render quad.
  RenderQuad* getRenderQuad() { return &m_RenderQuad; }

  // Push mesh to render.
  void pushMeshRender(MeshRenderCmd& cmd);
  void pushSimpleRender(SimpleRenderCmd& cmd);
  void pushDecal(DecalRenderCmd& cmd);
  void pushParticleSystem(ParticleSystem* system);
  void pushPointLight(const PointLight& lightInfo);
  void pushSpotLight(const SpotLight& lightInfo);
  void pushDirectionLight(const DirectionalLight& lightInfo);
  BufferUI* getUiBuffer() const;

  HDR* getHDR() { return m_pHDR; }

  // Get the name of the device used for rendering graphics and compute.
  const char* getDeviceName();

  // Adjusts bloom strength of the renderer.
  void adjustHDRSettings(const ParamsHDR& hdrSettings);

  void updateSky();

  // Set up and override Skybox cubemap for the renderer.
  void setSkyboxCubeMap(TextureCube* cubemap) { m_preRenderSkybox = cubemap; }
  void setGlobalBRDFLUT(Texture2D* brdflut) { m_skybox._brdfLUT = brdflut;}
  void setGlobalLightProbe(LightProbe* probe) { m_globalLightProbe = probe; }

  // NOTE(): If brdf, or envmap, was cleaned up before cleaning up the engine, be sure to 
  // call this first, before freeing the set maps!
  void              usePreRenderSkyboxMap(B32 enable);

  B32               usingPreRenderSkyboxMap() const { return m_usePreRenderSkybox; }

  // Builds/Updates commandbuffers for use in renderer. Very effective if you need to perform
  // a full update on the scene as a result of an application change, such as a window change. 
  // This will effectively stall the gpu if called too often. If you are adding/removing objects 
  // into a scene, use BuildAsync() instead.
  void              build();

  // Builds the commandbuffers asyncronously, this will prevent stalling the gpu rendering process
  // by using temporary commandbuffers and building them instead. When done, they will replace old 
  // commandbuffers. Use only if you need to update the scene as a result of dynamic objects/materials being
  // added/removed to/from the scene. Do not use this call if there is a window change or 
  // application change, as it will result warnings from the renderer!
  void              buildAsync();

  // Wipes out all currently pushed in meshes in this renderer.
  void              clearCmdLists();

  // Get backbuffer render width. This is different from window display width!
  U32 getRenderWidth() const { return m_renderWidth; }

  // Get backbuffer render height. This is different from window display height!
  U32 getRenderHeight() const { return m_renderHeight; }

  U32 getResourceBufferCount() const { return m_resourceBufferCount; }
  U32 getCurrentResourceBufferIndex() const { return m_currentResourceIndex; }

protected:
  // Start rendering onto a frame. This effectively querys for an available frame
  // to render onto.
  void              beginFrame();

  // Once frame rendering is done, call this function to submit back to the swapchain 
  // for presenting to the window.
  void              endFrame();

private:

  void              setUpFrameBuffers();
  void              setUpGraphicsPipelines();
  void              setUpDescriptorSetLayouts();
  void              cleanUpDescriptorSetLayouts();
  void              cleanUpGraphicsPipelines();
  void              cleanUpFrameBuffers();
  void              cleanUpRenderTextures(B32 fullCleanup);
  void              cleanUpOffscreen();
  void              cleanUpFinalOutputs();
  void              setUpFinalOutputs();
  void              setUpForwardPBR();
  void              setUpRenderTextures(B32 fullSetup);
  void              setUpOffscreen();
  void              setUpDebugPass();
  void              cleanUpDebugPass();
  void              generateDebugCmds();
  void              setUpPBR();
  void              setUpDescriptorSets();
  void              cleanUpDescriptorSets();
  void              setUpSkybox(B32 justSemaphores);
  void              generateOffScreenCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generatePbrCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generateShadowCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generateHDRCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generateSkyboxCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generateFinalCmds(CommandBuffer* buf);
  void              generateForwardPBRCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generateShadowResolveCmds(CommandBuffer* buf, U32 resourceIndex);
  void              generatePreZCmds(CommandBuffer* buf, U32 resourceIndex);
  void              updateRenderResolution(RenderResolution resolution);
  void              checkEnableLightShadows();

  void              buildOffScreenCmdList();
  void              buildPbrCmdLists();
  void              buildShadowCmdList();
  void              buildHDRCmdList();
  void              buildSkyboxCmdLists();
  void              buildForwardPBRCmdList();
  void              buildFinalCmdLists();
  void              updateGlobalIlluminationBuffer();
  void updateLightBuffer(U32 resourceIndex);

  void              setUpDownscale(B32 FullSetUp);
  void              cleanUpDownscale(B32 FullCleanUp);
  void              updateRuntimeConfigs(const GraphicsConfigParams* params);
  void              setUpHDR(B32 fullSetup);
  void              cleanUpForwardPBR();
  void              cleanUpHDR(B32 fullCleanup);
  void              cleanUpPBR();
  void              updateSkyboxCubeMap();
  void              cleanUpSkybox(B32 justSemaphores);
  void              updateSceneDescriptors(U32 resourceIndex);
  void              renderOverlay();
  void              renderPrimaryShadows();
  void              checkCmdUpdate(U32 frameIndex, U32 resourceIndex);
  void              setUpGlobalIlluminationBuffer();
  void              cleanUpGlobalIlluminationBuffer();
  void setBuffering(const GraphicsConfigParams* params);
  // Signals that tell if renderer needs to update any static data. Is cleaned out every 
  // frame!
  B32               staticNeedsUpdate() { return m_staticUpdate; }
  void              signalStaticUpdate() { m_staticUpdate = true; }
  void              cleanStaticUpdate() { m_staticUpdate = false; }

  void              sortCmdLists();
  void              waitForCpuFence();

  Window*           m_pWindow;

  // Command lists used by the renderer.
  CmdList<PrimitiveRenderCmd>            m_cmdDeferredList;
  CmdList<PrimitiveRenderCmd>            m_forwardCmdList;
  CmdList<PrimitiveRenderCmd>            m_staticCmdList;
  CmdList<PrimitiveRenderCmd>            m_dynamicCmdList;
  CmdList<SimpleRenderCmd>                m_debugRenderCmd;

  CmdList<JointDescriptor*>         m_jointDescriptors;
  CmdList<MeshDescriptor*>          m_meshDescriptors;
  CmdList<MaterialDescriptor*>      m_materialDescriptors;
  CmdList<ParticleSystem*>          m_particleSystems;
  CmdList<PointLight>               m_pointLights;
  CmdList<SpotLight>                m_spotLights;
  CmdList<DirectionalLight>         m_directionalLights;

  // Number of workers in this renderer instance. Used to enable multithreading.
  std::vector<std::thread>          m_workers;

  GlobalDescriptor* m_pGlobal;
  LightDescriptor*  m_pLights;

  // NOTE(): This can be abstracted, but we will be tight coupling with Vulkan anyway...
  VulkanRHI*        m_pRhi;

  struct {
    std::vector<CommandBuffer*>   _cmdBuffers;
    std::vector<CommandBuffer*>   _shadowCmdBuffers;
    std::vector<CommandBuffer*>   _shadowResolveCmdBuffers;
    std::vector<Semaphore*>       _semaphores;
    std::vector<Semaphore*>       _shadowSemaphores;
    std::vector<Semaphore*>       _resolveSemas;
  } m_Offscreen; 

  struct {
    std::vector<CommandBuffer*>   _CmdBuffers;
    std::vector<Semaphore*>       _Semas;
  } m_Pbr;

  struct {
    std::vector<CommandBuffer*>   _cmdBuffers;
    std::vector<Semaphore*>       _semaphores;
  } m_Forward;

  struct {
    ParamsHDR                     _pushCnst;
    std::vector<CommandBuffer*>   _CmdBuffers;
    std::vector<Semaphore*>       _semaphores;
    B32                           _Enabled;
  } m_HDR;

  struct {
    I32                           _Horizontal;
    R32                           _Strength;
    R32                           _Scale;
  } m_Downscale;

  struct {
    Texture2D*                    _envmap;
    Texture2D*                    _irradiance;
    Texture2D*                    _specular;
    Texture2D*                    _brdfLUT;
  } m_skybox;

  std::vector<CommandBuffer*>        m_pSkyboxCmdBuffers;
  std::vector<CommandBuffer*>        m_pFinalCommandBuffers;
  Fence*                m_cpuFence;
  std::vector<Semaphore*>  m_pFinalFinishedSemas;
  std::vector<Semaphore*>            m_SkyboxFinishedSignals;
  RenderQuad            m_RenderQuad;
  GraphicsConfigParams  m_currentGraphicsConfigs;
  UIOverlay*            m_pUI;
  DecalEngine*          m_decalEngine;
  ParticleEngine*       m_particleEngine;
  HDR*                  m_pHDR;
  SkyRenderer*          m_pSky;
  DebugManager*         m_pDebugManager;
  BakeIBL*              m_pBakeIbl;
  GlobalIllumination*   m_pGlobalIllumination;
  AntiAliasingFXAA*     m_pAntiAliasingFXAA;
  Clusterer*            m_pClusterer;
  TextureCube*          m_preRenderSkybox;
  LightProbe*           m_globalLightProbe;

  U32                   m_resourceBufferCount;
  U32                   m_currentResourceIndex;
  U32                   m_renderWidth;
  U32                   m_renderHeight;
  U32                   m_workGroupSize;
  U32                   m_rhiBits;
  B32                   m_staticUpdate;
  B32                   m_Rendering           : 1;
  B32                   m_Initialized         : 1;
  B32                   m_AntiAliasing        : 1; 
  B32                   m_Minimized           : 1;
  B32                   m_multithreaded       : 1;
  B32                   m_usePreRenderSkybox  : 1;
};

Renderer&           gRenderer();
} // Recluse