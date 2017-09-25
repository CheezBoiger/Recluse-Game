// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Utility/Module.hpp"
#include "Core/Utility/Vector.hpp"
#include "Core/Types.hpp"
#include "Core/Win32/Window.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"

#include "Core/Thread/Threading.hpp"

#include "Resources.hpp"


namespace Recluse {


class VulkanRHI;
class CmdList;
class DirectionLight;
class PointLight;
class SpotLight;
class LightProbe;
class ReflectionProbe;
class GraphicsPipeline;
class ComputePipeline;
class GpuParams;
class UserParams;
class Mesh;
class CubeMap;


// Renderer, which will be responsible for rendering out the scene from a
// camera's perspective.
class Renderer : public EngineModule<Renderer> {
public:
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
  void              AsyncPushCmdList(CmdList* cmdList);
  void              PushCmdList(CmdList* cmdList);

  void              BeginFrame();
  void              EndFrame();

  Mesh*             CreateMesh();
  DirectionLight*   CreateDirectionLight();
  PointLight*       CreatePointLight();
  SpotLight*        CreateSpotLight();

  CubeMap*          BakeEnvironmentMap(const Vector3& position);
  LightProbe*       BakeLightProbe(const CubeMap* envmap);
  Window*           WindowRef() { return mWindowHandle; }

private:
  Window*           mWindowHandle;
  CmdList*          mCmdList;

  // NOTE(): This can be abstracted, but we will be tight coupling with Vulkan anyway...
  VulkanRHI*        mRhi;

  // TODO(): We need to implement a pipeline map.
  resource_id_t     mClusterForwardPipeline;  
  resource_id_t     mSHCoefficentPrefilter;
  resource_id_t     mGeometryPipeline;
  resource_id_t     mPreprocessPipeline;
  resource_id_t     mHDRGammaPipeline;
  resource_id_t     mPostProcessPipelne;

  b8                mRendering;
};

Renderer&  gRenderer();
} // Recluse