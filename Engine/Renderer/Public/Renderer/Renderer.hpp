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

namespace Recluse {


class VulkanRHI;
class CmdList;
class LightProbe;
class CommandBuffer;
class ReflectionProbe;
class GraphicsPipeline;
class ComputePipeline;
class GpuParams;
class UserParams;
class MeshData;
class Material;
class LightDescriptor;
class GlobalDescriptor;
class TextureCube;
class Semaphore;
class Texture1D;
class Texture2D;
class Texture3D;
class Texture2DArray;
class TextureSampler;
class UIOverlay;
class RenderObject;

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

  // Set the light material for this renderer. This will set the lights that are in the world
  // scene. 
  void              SetLightMaterial(LightDescriptor*   material) { mLightDesc = material; }

  // Full RenderRHI wait til idle. This should not be called in time critical rendering.
  void              WaitIdle();

  // Creates a mesh object of which to submit to render.
  // Be sure to call FreeMeshData() if done with this mesh object.
  MeshData*         CreateMeshData();

  // Create a 1D texture.
  Texture1D*        CreateTexture1D();

  // Create a 2D Texture surface.
  Texture2D*        CreateTexture2D();

  // Create a 2D Array texture surface.
  Texture2DArray*   CreateTexture2DArray();

  Texture3D*        CreateTexture3D();
  TextureCube*      CreateTextureCube();
  TextureSampler*   CreateTextureSampler();

  // Create a light material object, which holds all lights that affect this 
  // scene. This will then be used for the light culling method of the renderer.
  LightDescriptor*    CreateLightMaterial();

  // Create a render object for the renderer to render?
  RenderObject*     CreateRenderObject();

  GlobalBuffer*     GlobalData() { return mGlobalDesc->Data(); }

  // Frees the render object.
  void              FreeRenderObject(RenderObject* renderObject);

  // Frees up the allocated mesh data object.
  void              FreeMeshData(MeshData* mesh);

  // Frees up the allocated light material object.
  void              FreeLightMaterial(LightDescriptor* material);

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
  void              CleanUpRenderTextures(b8 fullCleanup);
  void              CleanUpOffscreen(b8 fullCleanup);
  void              CleanUpFinalOutputs();
  void              SetUpFinalOutputs();
  void              SetUpRenderTextures(b8 fullSetup);
  void              SetUpOffscreen(b8 fullSetup);
  void              BuildOffScreenBuffer(u32 cmdBufferIndex);
  void              BuildHDRCmdBuffer(u32 cmdBufferIndex);
  void              SetUpDownscale(b8 FullSetUp);
  void              CleanUpDownscale(b8 FullCleanUp);
  void              SetUpHDR(b8 fullSetup);
  void              CleanUpHDR(b8 fullCleanup);
  void              UpdateMaterials();
  void              RenderOverlay();

  Window*           mWindowHandle;
  CmdList*          mCmdList;
  CmdList*          mDeferredCmdList;
  GlobalDescriptor*   mGlobalDesc;
  LightDescriptor*    mLightDesc;

  // NOTE(): This can be abstracted, but we will be tight coupling with Vulkan anyway...
  VulkanRHI*        mRhi;

  struct {
    std::vector<CommandBuffer*>   cmdBuffers;
    u32                           currCmdBufferIndex;
    Semaphore*                    semaphore;
  } mOffscreen; 

  struct {
    std::vector<CommandBuffer*>   cmdBuffers;
    u32                           currCmdBufferIndex;
    Semaphore*                    semaphore;
    b8                            enabled;
  } mHDR;

  struct {
    i32     horizontal;
    r32     strength;
    r32     scale;
  } m_Downscale;

  RenderQuad        mRenderQuad;
  UIOverlay*        mUI;
  b8                mRendering      : 1;
  b8                mInitialized    : 1;
};

Renderer&           gRenderer();
} // Recluse