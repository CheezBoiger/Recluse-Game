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
class TextureCube;
class Semaphore;
class Texture1D;
class Texture2D;
class Texture3D;
class Texture2DArray;
class TextureSampler;
class UIOverlay;

// Renderer, which will be responsible for rendering out the scene from a
// camera's perspective. Renderer is a module in charge of drawing and displaying
// onto a window surface. This module is important as it is the only way to see 
// stuff on screen, and to display pretty graphics!
class Renderer : public EngineModule<Renderer> {
public:
  // Definition of the UI Overlay for which to render onto.

  Renderer();
  ~Renderer();

  b8                Initialize(Window* window);
  b8                Rendering() const { return mRendering; }

  // Configure the renderer, resulting either add/removing features of the renderer such as 
  // certain pipelines like shadowing, or quality of the display. Pass nullptr in order 
  // just plain recreate the renderer scene.
  void              UpdateRendererConfigs(UserParams* params);

  // Clean up the renderer. This will "render" the renderer inactive.
  void              CleanUp();

  // Perform rendering of the display, you must call this function for each iteration in
  // the game loop!
  void              Render();

  // Callback used for EngineModule<> set up.
  void              OnStartUp() override;

  // Callback used for EngineModule<> set up.
  void              OnShutDown() override;

  // Push the command list into the renderer. This is the list used to figure out what 
  // renderable objects to draw and texture on the screen.
  void              PushCmdList(CmdList* cmdList) { mCmdList = cmdList; }

  // Push the deferred list into the renderer, this is the list used for commands to be called
  // after gbuffer offscreen rendering. This is mainly used for UI overlay rendering.
  void              PushDeferredCmdList(CmdList* cmdList) { mDeferredCmdList = cmdList; }

  // Builds/Updates commandbuffers for use in renderer. Very effective if you need to perform
  // a full update on the scene as a result of application change, such as a window change. 
  // This will effectively stall the gpu if called too often. If you need to constantly update 
  // the scene, use BuildAsync() instead.
  void              Build();
  
  // Builds the commandbuffers asyncronously, this will prevent stalling the gpu rendering process
  // by using temporary commandbuffers and building them instead. When done, they will replace old 
  // commandbuffers. Use only if you need to update the scene as a result of dynamic objects/materials being
  // added/removed to/from the scene. Do not use this call if there is a window change or 
  // application change, as it will result in a potential crash of the renderer!
  void              BuildAsync();

  // Set the global material for the renderer. This is the data used to specify  the world 
  // scene, which contains data about the current virtual camera, and global info of the world.
  void              SetGlobalMaterial(GlobalMaterial* material) { mGlobalMat = material; }

  // Set the light material for this renderer. This will set the lights that are in the world
  // scene. 
  void              SetLightMaterial(LightMaterial*   material) { mLightMat = material; }
  void              WaitIdle();

  // Creates a mesh object of which to submit to render.
  // Be sure to call FreeMesh() if done with this mesh object.
  Mesh*             CreateMesh();

  // Creates a material object to submit material samples of which to render a
  // Mesh. Be sure to call FreeMaterial() if done with this material object.
  Material*         CreateMaterial();

  Texture1D*        CreateTexture1D();
  Texture2D*        CreateTexture2D();
  Texture2DArray*   CreateTexture2DArray();
  Texture3D*        CreateTexture3D();
  TextureCube*      CreateTextureCube();
  TextureSampler*   CreateTextureSampler();

  // Create a global material object. This holds view, projection, SH coefficients
  // and other things that may affect the global aspect of the scene.
  GlobalMaterial*   CreateGlobalMaterial();
  
  // Create a light material object, which holds all lights that affect this 
  // scene. This will then be used for the light culling method of the renderer.
  LightMaterial*    CreateLightMaterial();

  // Create a directional light.
  DirectionLight*   CreateDirectionLight();

  // Create a point light.
  PointLight*       CreatePointLight();

  // Create a spotlight.
  SpotLight*        CreateSpotLight();

  // Frees up the allocated mesh object.
  void              FreeMesh(Mesh* mesh);

  // Frees up the allocated material object.
  void              FreeMaterial(Material* material);

  // Frees up the allocated global material object.
  void              FreeGlobalMaterial(GlobalMaterial* material);

  // Frees up the allocated light material object.
  void              FreeLightMaterial(LightMaterial* material);

  //  Frees up the allocated texture1d object.
  void              FreeTexture1D(Texture1D* texture);

  // Frees up the allocated texture2d object.
  void              FreeTexture2D(Texture2D* texture);

  // Frees up the allocated texture2darray object.
  void              FreeTexture2DArray(Texture2DArray* texture);

  // Frees up the allocated texture3d object.
  void              FreeTexture3D(Texture3D* texture);

  // Frees up the cubemap texture object.
  void              FreeTextureCube(TextureCube* texture);

  // Frees up the texture sampler object.
  void              FreeTextureSampler(TextureSampler* sampler);

  // Offline enviroment cube map baking. This is used for the surrounding 
  // scene around the mesh surface we are rendering.
  TextureCube*      BakeEnvironmentMap(const Vector3& position);

  // Offline light probe baking. We can effectively then use this probe in the scene
  // to render our mesh object with fast global illumination.
  LightProbe*       BakeLightProbe(const TextureCube* envmap);

  // Window reference.
  Window*           WindowRef() { return mWindowHandle; }
  UIOverlay*        Overlay() { return mUI; }  

  // Check if this renderer is initialized with the window reference given.
  b8                Initialized() { return mInitialized; }

  // Get the rendering hardware interface used in this renderer.
  VulkanRHI*        RHI() { return mRhi; }

  void              SetGamma(r32 gamma);
  void              SetExposure(r32 exposure);
  r32               Gamma() const { return mHDR.data.gamma; }
  r32               Exposure() const { return mHDR.data.exposure; }

  void              EnableHDR(b8 enable);

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
  void              CleanUpRenderTextures();
  void              CleanUpOffscreen(b8 fullCleanup);
  void              CleanUpFinalOutputs();
  void              SetUpFinalOutputs();
  void              SetUpRenderTextures();
  void              SetUpOffscreen(b8 fullSetup);
  void              BuildOffScreenBuffer(u32 cmdBufferIndex);
  void              BuildHDRCmdBuffer(u32 cmdBufferIndex);
  void              SetUpHDR(b8 fullSetup);
  void              CleanUpHDR(b8 fullCleanup);
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
    std::vector<CommandBuffer*>   cmdBuffers;
    u32                           currCmdBufferIndex;
    Semaphore*                    semaphore;
  } mOffscreen; 

  struct HDRBuffer {
    r32       gamma;
    r32       exposure;
    r32       pad0[2];
    i32       bloomEnabled;
    i32       pad1[3];
  };

  struct {
    std::vector<CommandBuffer*>   cmdBuffers;
    HDRBuffer                     data;
    u32                           currCmdBufferIndex;
    Semaphore*                    semaphore;
    Buffer*                       hdrBuffer;
    b8                            enabled;
  } mHDR;

  ScreenQuad        mScreenQuad;
  UIOverlay*        mUI;
  b8                mRendering;
  b8                mInitialized;
};

Renderer&           gRenderer();
} // Recluse